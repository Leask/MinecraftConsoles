#include "DedicatedServerHeadlessRuntime.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "DedicatedServerPlatformRuntime.h"
#include "DedicatedServerSignalState.h"
#include "DedicatedServerSocketBootstrap.h"
#include "../ServerLogger.h"
#include "lce_net/lce_net.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"

namespace
{
    class DedicatedServerHeadlessRuntimeGuard
    {
    public:
        DedicatedServerHeadlessRuntimeGuard() = default;

        ~DedicatedServerHeadlessRuntimeGuard()
        {
            Shutdown();
        }

        void MarkPlatformRuntimeStarted()
        {
            m_platformRuntimeStarted = true;
        }

        void MarkSocketRuntimeStarted()
        {
            m_socketRuntimeStarted = true;
        }

        void MarkSocketBootstrapStarted()
        {
            m_socketBootstrapStarted = true;
        }

        ServerRuntime::DedicatedServerSocketBootstrapState *GetSocketState()
        {
            return &m_socketState;
        }

        void Shutdown()
        {
            if (m_socketBootstrapStarted)
            {
                ServerRuntime::StopDedicatedServerSocketBootstrap(
                    &m_socketState);
                m_socketBootstrapStarted = false;
            }

            if (m_socketRuntimeStarted)
            {
                LceNetShutdown();
                m_socketRuntimeStarted = false;
            }

            if (m_platformRuntimeStarted)
            {
                ServerRuntime::StopDedicatedServerPlatformRuntime();
                m_platformRuntimeStarted = false;
            }
        }

    private:
        bool m_platformRuntimeStarted = false;
        bool m_socketRuntimeStarted = false;
        bool m_socketBootstrapStarted = false;
        ServerRuntime::DedicatedServerSocketBootstrapState m_socketState = {};
    };

    const char *GetDedicatedServerLoopbackTarget(
        const std::string &bindIp)
    {
        return bindIp == "0.0.0.0"
            ? "127.0.0.1"
            : bindIp.c_str();
    }

    int RunDedicatedServerSelfConnect(
        const ServerRuntime::DedicatedServerBootstrapContext &context,
        const ServerRuntime::DedicatedServerSocketBootstrapState &socketState)
    {
        const char *loopbackTarget = GetDedicatedServerLoopbackTarget(
            context.runtimeState.bindIp);
        LceSocketHandle clientSocket = LceNetOpenTcpSocket();
        if (clientSocket == LCE_INVALID_SOCKET)
        {
            ServerRuntime::LogError(
                "startup",
                "failed to allocate self-connect client socket");
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
            return 8;
        }

        ServerRuntime::LogInfof(
            "startup",
            "native bootstrap self-connect accepted from %s:%d",
            acceptedIp,
            acceptedPort);
        LceNetCloseSocket(acceptedSocket);
        LceNetCloseSocket(clientSocket);
        return 0;
    }

    void RunDedicatedServerHeadlessShell()
    {
        ServerRuntime::LogInfo(
            "startup",
            "native bootstrap shell running; "
            "type stop or send SIGINT to exit");
        while (!ServerRuntime::IsDedicatedServerShutdownRequested())
        {
            if (LceStdinIsAvailable() && LceWaitForStdinReadable(50) == 1)
            {
                char lineBuffer[256] = {};
                if (std::fgets(lineBuffer, sizeof(lineBuffer), stdin) != nullptr)
                {
                    const std::string line = lineBuffer;
                    if (line == "stop\n" ||
                        line == "stop\r\n" ||
                        line == "stop")
                    {
                        ServerRuntime::RequestDedicatedServerShutdown();
                        break;
                    }
                }
            }

            LceSleepMilliseconds(50);
        }
    }
}

namespace ServerRuntime
{
    int RunDedicatedServerHeadlessRuntime(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        const DedicatedServerHeadlessRuntimeOptions &options)
    {
        DedicatedServerHeadlessRuntimeGuard runtimeGuard;
        const DedicatedServerPlatformRuntimeStartResult
            platformRuntimeStartResult =
                StartDedicatedServerPlatformRuntime(platformState);
        if (!platformRuntimeStartResult.ok)
        {
            LogError(
                "startup",
                platformRuntimeStartResult.errorMessage.c_str());
            return platformRuntimeStartResult.exitCode;
        }

        runtimeGuard.MarkPlatformRuntimeStarted();
        LogInfof(
            "startup",
            "native platform runtime=%s headless=%s",
            platformRuntimeStartResult.runtimeName.c_str(),
            platformRuntimeStartResult.headless ? "true" : "false");

        if (options.bootstrapOnly)
        {
            LogInfo(
                "startup",
                "native bootstrap completed in bootstrap-only mode");
            return 0;
        }

        if (!LceNetInitialize())
        {
            LogError(
                "startup",
                "failed to initialize native socket runtime");
            return 4;
        }

        runtimeGuard.MarkSocketRuntimeStarted();
        std::string socketError;
        if (!StartDedicatedServerSocketBootstrap(
                context,
                runtimeGuard.GetSocketState(),
                &socketError))
        {
            LogError("startup", socketError.c_str());
            return 5;
        }

        runtimeGuard.MarkSocketBootstrapStarted();
        LogInfof(
            "startup",
            "native bootstrap bound dedicated listener on %s:%d",
            context.runtimeState.bindIp.c_str(),
            runtimeGuard.GetSocketState()->boundPort);

        if (options.selfConnect)
        {
            const int selfConnectExitCode = RunDedicatedServerSelfConnect(
                context,
                *runtimeGuard.GetSocketState());
            if (selfConnectExitCode == 0)
            {
                LogInfo("shutdown", "native bootstrap self-connect ok");
            }
            return selfConnectExitCode;
        }

        RunDedicatedServerHeadlessShell();
        LogInfo("shutdown", "native bootstrap shell stopped");
        return 0;
    }
}
