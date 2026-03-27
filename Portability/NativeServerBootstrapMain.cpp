#include <atomic>
#include <cstdint>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Minecraft.Server/Common/DedicatedServerBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerOptions.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformState.h"
#include "Minecraft.Server/Common/DedicatedServerSocketBootstrap.h"
#include "Minecraft.Server/ServerLogger.h"
#include "lce_net/lce_net.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"

namespace
{
    std::atomic<bool> g_shutdownRequested(false);

    void RequestShutdown()
    {
        g_shutdownRequested.store(true);
    }

    void NativeSignalHandler(int)
    {
        RequestShutdown();
    }

    const char* GetDedicatedServerLoopbackTarget(
        const std::string &bindIp)
    {
        return bindIp == "0.0.0.0"
            ? "127.0.0.1"
            : bindIp.c_str();
    }
}

int main(int argc, char** argv)
{
    bool bootstrapOnly = false;
    bool selfConnect = false;
    int serverArgc = 1;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--bootstrap-only") == 0)
        {
            bootstrapOnly = true;
            continue;
        }
        if (std::strcmp(argv[i], "--self-connect") == 0)
        {
            selfConnect = true;
            continue;
        }

        argv[serverArgc++] = argv[i];
    }

    std::signal(SIGINT, NativeSignalHandler);
    std::signal(SIGTERM, NativeSignalHandler);

    ServerRuntime::DedicatedServerBootstrapContext bootstrapContext = {};
    std::string parseError;
    switch (ServerRuntime::PrepareDedicatedServerBootstrapContext(
            serverArgc,
            argv,
            &bootstrapContext,
            &parseError))
    {
    case ServerRuntime::eDedicatedServerBootstrap_Ready:
        break;
    case ServerRuntime::eDedicatedServerBootstrap_ShowHelp:
        {
            std::vector<std::string> usageLines;
            ServerRuntime::BuildDedicatedServerUsageLines(&usageLines);
            for (const std::string& line : usageLines)
            {
                std::printf("%s\n", line.c_str());
            }
            return 0;
        }
    case ServerRuntime::eDedicatedServerBootstrap_Failed:
    default:
        if (!parseError.empty())
        {
            std::fprintf(stderr, "startup error: %s\n", parseError.c_str());
        }
        return 2;
    }

    std::string bootstrapError;
    if (!ServerRuntime::InitializeDedicatedServerBootstrapEnvironment(
            &bootstrapContext,
            &bootstrapError))
    {
        ServerRuntime::LogError("startup", bootstrapError.c_str());
        return 3;
    }

    ServerRuntime::LogDedicatedServerBootstrapSummary(
        "native bootstrap",
        bootstrapContext);
    const ServerRuntime::DedicatedServerPlatformState platformState =
        ServerRuntime::BuildDedicatedServerPlatformState(
            bootstrapContext.runtimeState,
            1);
    const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
        platformRuntimeStartResult =
            ServerRuntime::StartDedicatedServerPlatformRuntime(
                platformState);
    if (!platformRuntimeStartResult.ok)
    {
        ServerRuntime::LogError(
            "startup",
            platformRuntimeStartResult.errorMessage.c_str());
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapContext);
        return platformRuntimeStartResult.exitCode;
    }
    ServerRuntime::LogInfof(
        "startup",
        "native platform runtime=%s headless=%s",
        platformRuntimeStartResult.runtimeName.c_str(),
        platformRuntimeStartResult.headless ? "true" : "false");

    if (bootstrapOnly)
    {
        ServerRuntime::LogInfo(
            "startup",
            "native bootstrap completed in bootstrap-only mode");
        ServerRuntime::StopDedicatedServerPlatformRuntime();
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapContext);
        return 0;
    }

    if (!LceNetInitialize())
    {
        ServerRuntime::LogError(
            "startup",
            "failed to initialize native socket runtime");
        ServerRuntime::StopDedicatedServerPlatformRuntime();
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapContext);
        return 4;
    }

    ServerRuntime::DedicatedServerSocketBootstrapState socketState = {};
    std::string socketError;
    if (!ServerRuntime::StartDedicatedServerSocketBootstrap(
            bootstrapContext,
            &socketState,
            &socketError))
    {
        ServerRuntime::LogError("startup", socketError.c_str());
        LceNetShutdown();
        ServerRuntime::StopDedicatedServerPlatformRuntime();
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapContext);
        return 5;
    }

    ServerRuntime::LogInfof(
        "startup",
        "native bootstrap bound dedicated listener on %s:%d",
        bootstrapContext.runtimeState.bindIp.c_str(),
        socketState.boundPort);
    if (selfConnect)
    {
        const char* loopbackTarget = GetDedicatedServerLoopbackTarget(
            bootstrapContext.runtimeState.bindIp);
        LceSocketHandle clientSocket = LceNetOpenTcpSocket();
        if (clientSocket == LCE_INVALID_SOCKET)
        {
            ServerRuntime::LogError(
                "startup",
                "failed to allocate self-connect client socket");
            ServerRuntime::StopDedicatedServerSocketBootstrap(&socketState);
            LceNetShutdown();
            ServerRuntime::StopDedicatedServerPlatformRuntime();
            ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
                &bootstrapContext);
            return 6;
        }

        if (!LceNetConnectIpv4(
                clientSocket,
                loopbackTarget,
                socketState.boundPort))
        {
            LceNetCloseSocket(clientSocket);
            ServerRuntime::LogErrorf(
                "startup",
                "failed to self-connect to %s:%d",
                loopbackTarget,
                socketState.boundPort);
            ServerRuntime::StopDedicatedServerSocketBootstrap(&socketState);
            LceNetShutdown();
            ServerRuntime::StopDedicatedServerPlatformRuntime();
            ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
                &bootstrapContext);
            return 7;
        }

        char acceptedIp[64] = {};
        int acceptedPort = -1;
        LceSocketHandle acceptedSocket = LceNetAcceptIpv4(
            socketState.listener,
            acceptedIp,
            sizeof(acceptedIp),
            &acceptedPort);
        if (acceptedSocket == LCE_INVALID_SOCKET)
        {
            LceNetCloseSocket(clientSocket);
            ServerRuntime::LogError(
                "startup",
                "failed to accept self-connect socket");
            ServerRuntime::StopDedicatedServerSocketBootstrap(&socketState);
            LceNetShutdown();
            ServerRuntime::StopDedicatedServerPlatformRuntime();
            ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
                &bootstrapContext);
            return 8;
        }

        ServerRuntime::LogInfof(
            "startup",
            "native bootstrap self-connect accepted from %s:%d",
            acceptedIp,
            acceptedPort);
        LceNetCloseSocket(acceptedSocket);
        LceNetCloseSocket(clientSocket);
        ServerRuntime::StopDedicatedServerSocketBootstrap(&socketState);
        LceNetShutdown();
        ServerRuntime::StopDedicatedServerPlatformRuntime();
        ServerRuntime::LogInfo("shutdown", "native bootstrap self-connect ok");
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapContext);
        return 0;
    }

    ServerRuntime::LogInfo(
        "startup",
        "native bootstrap shell running; type stop or send SIGINT to exit");
    while (!g_shutdownRequested.load())
    {
        if (LceStdinIsAvailable() && LceWaitForStdinReadable(50) == 1)
        {
            char lineBuffer[256] = {};
            if (std::fgets(lineBuffer, sizeof(lineBuffer), stdin) != nullptr)
            {
                const std::string line = lineBuffer;
                if (line == "stop\n" || line == "stop\r\n" || line == "stop")
                {
                    RequestShutdown();
                    break;
                }
            }
        }

        LceSleepMilliseconds(50);
    }

    ServerRuntime::StopDedicatedServerSocketBootstrap(&socketState);
    LceNetShutdown();
    ServerRuntime::StopDedicatedServerPlatformRuntime();
    ServerRuntime::LogInfo("shutdown", "native bootstrap shell stopped");
    ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
        &bootstrapContext);
    return 0;
}
