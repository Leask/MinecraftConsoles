#include <atomic>
#include <cstdint>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Minecraft.Server/Common/DedicatedServerBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerOptions.h"
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
}

int main(int argc, char** argv)
{
    bool bootstrapOnly = false;
    int serverArgc = 1;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--bootstrap-only") == 0)
        {
            bootstrapOnly = true;
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

    if (bootstrapOnly)
    {
        ServerRuntime::LogInfo(
            "startup",
            "native bootstrap completed in bootstrap-only mode");
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapContext);
        return 0;
    }

    if (!LceNetInitialize())
    {
        ServerRuntime::LogError(
            "startup",
            "failed to initialize native socket runtime");
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
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapContext);
        return 5;
    }

    ServerRuntime::LogInfof(
        "startup",
        "native bootstrap bound dedicated listener on %s:%d",
        bootstrapContext.runtimeState.bindIp.c_str(),
        socketState.boundPort);
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
    ServerRuntime::LogInfo("shutdown", "native bootstrap shell stopped");
    ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
        &bootstrapContext);
    return 0;
}
