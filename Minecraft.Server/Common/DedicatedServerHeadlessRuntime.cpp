#include "DedicatedServerHeadlessRuntime.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "DedicatedServerHeadlessShell.h"
#include "DedicatedServerHostedGameRuntimeState.h"
#include "DedicatedServerLifecycle.h"
#include "NativeDedicatedServerHostedGameRuntimeStub.h"
#include "DedicatedServerPlatformRuntime.h"
#include "DedicatedServerSignalState.h"
#include "DedicatedServerSocketBootstrap.h"
#include "FileUtils.h"
#include "NativeDedicatedServerLoadedSaveState.h"
#include "NativeDedicatedServerSaveStub.h"
#include "Portability/NativeDedicatedServerHostedGameSession.h"
#include "../ServerLogger.h"
#include "../WorldManager.h"
#include "lce_net/lce_net.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"

namespace
{
    static constexpr int kDedicatedServerStartupFailureExitCode = 4;

    struct DedicatedServerHeadlessRunHooksContext
    {
        ServerRuntime::DedicatedServerBootstrapContext *bootstrapContext =
            nullptr;
        const ServerRuntime::DedicatedServerPlatformState *platformState =
            nullptr;
        const ServerRuntime::DedicatedServerHeadlessRuntimeOptions *options =
            nullptr;
        ServerRuntime::DedicatedServerHeadlessShellContext shellContext = {};
        ServerRuntime::DedicatedServerHeadlessShellState shellState = {};
        LceSocketHandle listener = LCE_INVALID_SOCKET;
        int listenerPort = 0;
        std::uint64_t shellStartMs = 0;
        std::uint64_t persistedAutosaveCompletions = 0;
        int failureExitCode = 0;
    };

    struct DedicatedServerHeadlessWorldBootstrapResult
    {
        int exitCode = 0;
        ServerRuntime::WorldBootstrapResult worldBootstrap = {};
    };

    class DedicatedServerHeadlessWorldBootstrapGuard
    {
    public:
        explicit DedicatedServerHeadlessWorldBootstrapGuard(
            ServerRuntime::WorldBootstrapResult *worldBootstrap)
            : m_worldBootstrap(worldBootstrap)
        {
        }

        ~DedicatedServerHeadlessWorldBootstrapGuard()
        {
            if (m_worldBootstrap == nullptr)
            {
                return;
            }

            ServerRuntime::DestroyLoadSaveDataThreadParam(
                m_worldBootstrap->saveData);
            m_worldBootstrap->saveData = nullptr;
        }

    private:
        ServerRuntime::WorldBootstrapResult *m_worldBootstrap = nullptr;
    };

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

    std::string BuildDedicatedServerHeadlessSavePath(
        const DedicatedServerHeadlessRunHooksContext &context);

    void RefreshDedicatedServerHeadlessShellContext(
        DedicatedServerHeadlessRunHooksContext *context);

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
        const LceSocketHandle acceptedSocket = LceNetAcceptIpv4(
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

    bool InitiateDedicatedServerShellSelfConnect(
        const ServerRuntime::DedicatedServerBootstrapContext &context,
        int listenerPort,
        const std::vector<std::string> &commands)
    {
        const char *loopbackTarget = GetDedicatedServerLoopbackTarget(
            context.runtimeState.bindIp);
        LceSocketHandle clientSocket = LceNetOpenTcpSocket();
        if (clientSocket == LCE_INVALID_SOCKET)
        {
            ServerRuntime::LogError(
                "startup",
                "failed to allocate shell self-connect client socket");
            return false;
        }

        const bool connected = LceNetConnectIpv4(
            clientSocket,
            loopbackTarget,
            listenerPort);
        if (!connected)
        {
            ServerRuntime::LogErrorf(
                "startup",
                "failed to initiate shell self-connect to %s:%d",
                loopbackTarget,
                listenerPort);
        }
        else if (!commands.empty())
        {
            std::string requestPayload;
            for (size_t i = 0; i < commands.size(); ++i)
            {
                requestPayload.append(commands[i]);
                requestPayload.push_back('\n');
            }

            if (!requestPayload.empty() &&
                !LceNetSendAll(
                    clientSocket,
                    requestPayload.data(),
                    (int)requestPayload.size()))
            {
                ServerRuntime::LogError(
                    "startup",
                    "failed to send shell self-connect commands");
                LceNetCloseSocket(clientSocket);
                return false;
            }
        }

        LceNetCloseSocket(clientSocket);
        return connected;
    }

    ServerRuntime::DedicatedServerHostedGameRuntimeSessionContext
    BuildDedicatedServerHostedGameRuntimeSessionContext(
        const DedicatedServerHeadlessRunHooksContext &context)
    {
        ServerRuntime::DedicatedServerHostedGameRuntimeSessionContext
            sessionContext = {};
        sessionContext.worldName = context.shellContext.worldName;
        sessionContext.worldSaveId = context.shellContext.worldSaveId;
        sessionContext.savePath = BuildDedicatedServerHeadlessSavePath(context);
        sessionContext.storageRoot = context.shellContext.storageRoot;
        sessionContext.hostName = context.shellContext.hostName;
        sessionContext.bindIp = context.shellContext.bindIp;
        sessionContext.configuredPort = context.shellContext.multiplayerPort;
        sessionContext.listenerPort = context.listenerPort;
        return sessionContext;
    }

    std::string GetDedicatedServerHeadlessSaveId(
        const DedicatedServerHeadlessRunHooksContext &context)
    {
        if (!context.shellContext.worldSaveId.empty())
        {
            return context.shellContext.worldSaveId;
        }

        return "world";
    }

    std::string BuildDedicatedServerHeadlessSavePath(
        const DedicatedServerHeadlessRunHooksContext &context)
    {
        const ServerRuntime::NativeDedicatedServerLoadedSaveMetadata
            loadedSaveMetadata =
                ServerRuntime::GetNativeDedicatedServerLoadedSaveMetadata();
        if (loadedSaveMetadata.available &&
            !loadedSaveMetadata.savePath.empty())
        {
            return loadedSaveMetadata.savePath;
        }

        std::string savePath = context.shellContext.storageRoot;
        if (!savePath.empty() && savePath.back() != '/')
        {
            savePath.push_back('/');
        }

        savePath.append(GetDedicatedServerHeadlessSaveId(context));
        savePath.append(".save");
        return savePath;
    }

    bool PersistDedicatedServerHeadlessSaveStub(
        DedicatedServerHeadlessRunHooksContext *context,
        bool forcePersist = false)
    {
        if (context == nullptr)
        {
            return false;
        }

        const std::uint64_t completedAutosaves =
            ServerRuntime::GetDedicatedServerAutosaveCompletionCount();
        if (!forcePersist &&
            completedAutosaves <= context->persistedAutosaveCompletions)
        {
            return true;
        }

        RefreshDedicatedServerHeadlessShellContext(context);
        ServerRuntime::ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
            LceGetMonotonicMilliseconds());
        const std::string savePath =
            BuildDedicatedServerHeadlessSavePath(*context);
        const std::uint64_t nowMs = LceGetMonotonicMilliseconds();
        ServerRuntime::NativeDedicatedServerHostedGameSessionPersistContext
            persistContext = {};
        persistContext.worldName = context->shellContext.worldName;
        persistContext.worldSaveId =
            GetDedicatedServerHeadlessSaveId(*context);
        persistContext.hostName = context->shellContext.hostName;
        persistContext.bindIp = context->shellContext.bindIp;
        persistContext.configuredPort =
            context->shellContext.multiplayerPort;
        persistContext.listenerPort = context->shellContext.listenerPort;
        ServerRuntime::NativeDedicatedServerSaveStub saveStub = {};
        if (!ServerRuntime::BuildNativeDedicatedServerSaveStubFromSessionSnapshot(
                persistContext,
                nowMs,
                ServerRuntime::FileUtils::GetCurrentUtcFileTime(),
                &saveStub))
        {
            ServerRuntime::LogWarn(
                "world-io",
                "failed to build native save stub from session core");
            return false;
        }

        std::string saveText;
        if (!ServerRuntime::BuildNativeDedicatedServerSaveStubText(
                saveStub,
                &saveText) ||
            !ServerRuntime::FileUtils::WriteTextFileAtomic(
                savePath,
                saveText))
        {
            ServerRuntime::LogWarnf(
                "world-io",
                "failed to persist native save stub %s",
                savePath.c_str());
            return false;
        }

        context->persistedAutosaveCompletions = completedAutosaves;
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionPersistedSaveAndProject(
            savePath,
            saveStub.savedAtFileTime,
            completedAutosaves,
            LceGetMonotonicMilliseconds());
        ServerRuntime::LogInfof(
            "world-io",
            "persisted native save stub #%llu to %s",
            (unsigned long long)completedAutosaves,
            savePath.c_str());
        return true;
    }

    void RefreshDedicatedServerHeadlessShellContext(
        DedicatedServerHeadlessRunHooksContext *context)
    {
        if (context == nullptr ||
            context->bootstrapContext == nullptr ||
            context->platformState == nullptr)
        {
            return;
        }

        context->shellContext =
            ServerRuntime::BuildDedicatedServerHeadlessShellContext(
                *context->bootstrapContext,
                *context->platformState,
                context->listenerPort);
    }

    void StartDedicatedServerHeadlessShellHook(void *hookContext)
    {
        DedicatedServerHeadlessRunHooksContext *context =
            static_cast<DedicatedServerHeadlessRunHooksContext *>(hookContext);
        if (context == nullptr)
        {
            return;
        }

        RefreshDedicatedServerHeadlessShellContext(context);
        context->shellStartMs = LceGetMonotonicMilliseconds();
        const ServerRuntime::DedicatedServerHostedGameRuntimeSessionContext
            sessionContext =
                BuildDedicatedServerHostedGameRuntimeSessionContext(*context);
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionContextAndProject(
            sessionContext.worldName,
            sessionContext.worldSaveId,
            sessionContext.savePath,
            sessionContext.storageRoot,
            sessionContext.hostName,
            sessionContext.bindIp,
            sessionContext.configuredPort,
            sessionContext.listenerPort,
            context->shellStartMs,
            context->shellStartMs);
        ServerRuntime::LogInfo(
            "startup",
            "native bootstrap shell running; "
            "type stop or send SIGINT to exit");
    }

    void RunDedicatedServerHeadlessShellCommandsHook(void *hookContext)
    {
        DedicatedServerHeadlessRunHooksContext *context =
            static_cast<DedicatedServerHeadlessRunHooksContext *>(hookContext);
        if (context == nullptr || context->options == nullptr)
        {
            return;
        }

        RefreshDedicatedServerHeadlessShellContext(context);
        if (context->options->shellSelfConnect &&
            !InitiateDedicatedServerShellSelfConnect(
                *context->bootstrapContext,
                context->listenerPort,
                context->options->shellSelfCommands))
        {
            context->failureExitCode = 9;
            ServerRuntime::RequestDedicatedServerShutdown();
            return;
        }

        for (size_t i = 0; i < context->options->scriptedCommands.size(); ++i)
        {
            ServerRuntime::ExecuteDedicatedServerHeadlessShellCommand(
                context->options->scriptedCommands[i],
                context->shellContext,
                context->shellState);
            if (ServerRuntime::IsDedicatedServerShutdownRequested())
            {
                break;
            }
        }
    }

    void PollDedicatedServerHeadlessShellHook(void *hookContext)
    {
        DedicatedServerHeadlessRunHooksContext *context =
            static_cast<DedicatedServerHeadlessRunHooksContext *>(hookContext);
        if (context == nullptr || context->options == nullptr)
        {
            return;
        }

        ServerRuntime::PollDedicatedServerHeadlessShellConnections(
            context->listener,
            context->shellContext,
            &context->shellState);
        const std::uint64_t now = LceGetMonotonicMilliseconds();

        if (context->options->shutdownAfterMs > 0)
        {
            if (now - context->shellStartMs >=
                context->options->shutdownAfterMs)
            {
                ServerRuntime::LogInfof(
                    "shutdown",
                    "native bootstrap auto-shutdown after %llums",
                    (unsigned long long)context->options->shutdownAfterMs);
                ServerRuntime::RequestDedicatedServerShutdown();
                return;
            }
        }

        if (LceStdinIsAvailable() && LceWaitForStdinReadable(0) == 1)
        {
            char lineBuffer[256] = {};
            if (std::fgets(lineBuffer, sizeof(lineBuffer), stdin) != nullptr)
            {
                ServerRuntime::ExecuteDedicatedServerHeadlessShellCommand(
                    lineBuffer,
                    context->shellContext,
                    context->shellState);
            }
        }

        PersistDedicatedServerHeadlessSaveStub(context);
    }

    bool ValidateDedicatedServerHeadlessShellRun(
        const ServerRuntime::DedicatedServerHeadlessRuntimeOptions &options,
        const DedicatedServerHeadlessRunHooksContext &context)
    {
        const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot =
                ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
        if (context.failureExitCode != 0)
        {
            return false;
        }

        if (options.requiredRemoteCommands > 0 &&
            sessionSnapshot.remoteCommands <
                options.requiredRemoteCommands)
        {
            ServerRuntime::LogErrorf(
                "network",
                "native shell expected at least %llu remote "
                "commands but only observed %llu",
                (unsigned long long)options.requiredRemoteCommands,
                (unsigned long long)sessionSnapshot.remoteCommands);
            return false;
        }

        if (options.requiredAcceptedConnections == 0)
        {
            return true;
        }

        if (sessionSnapshot.acceptedConnections >=
            options.requiredAcceptedConnections)
        {
            return true;
        }

        ServerRuntime::LogErrorf(
            "network",
            "native shell expected at least %llu accepted "
            "connections but only observed %llu",
            (unsigned long long)options.requiredAcceptedConnections,
            (unsigned long long)sessionSnapshot.acceptedConnections);
        return false;
    }

    DedicatedServerHeadlessWorldBootstrapResult
    BootstrapDedicatedServerHeadlessWorld(
        const ServerRuntime::DedicatedServerBootstrapContext &context)
    {
        DedicatedServerHeadlessWorldBootstrapResult result = {};
        ServerRuntime::ResetNativeDedicatedServerLoadedSaveMetadata();
        result.worldBootstrap = ServerRuntime::BootstrapWorldForServer(
            context.serverProperties,
            0,
            nullptr);
        if (result.worldBootstrap.status ==
            ServerRuntime::eWorldBootstrap_Failed)
        {
            ServerRuntime::LogError(
                "startup",
                "native world bootstrap failed");
            result.exitCode = kDedicatedServerStartupFailureExitCode;
            return result;
        }

        const char *bootstrapMode =
            result.worldBootstrap.status == ServerRuntime::eWorldBootstrap_Loaded
                ? "loaded"
                : "created-new";
        ServerRuntime::LogInfof(
            "startup",
            "native world bootstrap=%s level-id=%s",
            bootstrapMode,
            result.worldBootstrap.resolvedSaveId.c_str());

        if (result.worldBootstrap.saveData != nullptr)
        {
            ServerRuntime::LogInfo(
                "startup",
                "native stub runtime prepared loaded save payload");
            const ServerRuntime::NativeDedicatedServerLoadedSaveMetadata
                loadedSaveMetadata =
                    ServerRuntime::GetNativeDedicatedServerLoadedSaveMetadata();
            if (loadedSaveMetadata.available)
            {
                ServerRuntime::LogInfof(
                    "startup",
                    "native loaded save metadata path=%s startup=%s "
                    "phase=%s settings=0x%08x no-local=%s world-size=%u "
                    "hell-scale=%u online=%s private=%s fake-local=%s "
                    "public-slots=%u private-slots=%u "
                    "payload-checksum=0x%016llx save-generation=%llu "
                    "state-checksum=0x%016llx startup-payload=%s "
                    "validated=%s startup-steps=%llu startup-ms=%llu "
                    "remote=%llu autosaves=%llu worker-pending=%llu "
                    "worker-ticks=%llu worker-completions=%llu "
                    "ticks=%llu uptime=%llu completed=%s "
                    "app-shutdown=%s shutdown-halted=%s "
                    "gameplay-iterations=%llu hosted-thread=%s "
                    "hosted-thread-ticks=%llu",
                    loadedSaveMetadata.savePath.c_str(),
                    loadedSaveMetadata.saveStub.startupMode.c_str(),
                    loadedSaveMetadata.saveStub.sessionPhase.c_str(),
                    (unsigned int)
                        loadedSaveMetadata.saveStub.hostSettings,
                    loadedSaveMetadata.saveStub.dedicatedNoLocalHostPlayer
                        ? "true"
                        : "false",
                    loadedSaveMetadata.saveStub.worldSizeChunks,
                    loadedSaveMetadata.saveStub.worldHellScale,
                    loadedSaveMetadata.saveStub.onlineGame ? "true" : "false",
                    loadedSaveMetadata.saveStub.privateGame ? "true" : "false",
                    loadedSaveMetadata.saveStub.fakeLocalPlayerJoined
                        ? "true"
                        : "false",
                    loadedSaveMetadata.saveStub.publicSlots,
                    loadedSaveMetadata.saveStub.privateSlots,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.payloadChecksum,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.saveGeneration,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.stateChecksum,
                    loadedSaveMetadata.saveStub.startupPayloadPresent
                        ? "present"
                        : "none",
                    loadedSaveMetadata.saveStub.startupPayloadValidated
                        ? "true"
                        : "false",
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.startupThreadIterations,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.startupThreadDurationMs,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.remoteCommands,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.autosaveCompletions,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub
                            .workerPendingWorldActionTicks,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.workerTickCount,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub
                            .completedWorkerActions,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.platformTickCount,
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.uptimeMs,
                    loadedSaveMetadata.saveStub.sessionCompleted
                        ? "true"
                        : "false",
                    loadedSaveMetadata.saveStub.requestedAppShutdown
                        ? "true"
                        : "false",
                    loadedSaveMetadata.saveStub.shutdownHaltedGameplay
                        ? "true"
                        : "false",
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.gameplayLoopIterations,
                    loadedSaveMetadata.saveStub.hostedThreadActive
                        ? "active"
                        : "stopped",
                    (unsigned long long)
                        loadedSaveMetadata.saveStub.hostedThreadTicks);
            }
        }

        return result;
    }
}

namespace ServerRuntime
{
    int RunDedicatedServerHeadlessRuntime(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        const DedicatedServerHeadlessRuntimeOptions &options)
    {
        DedicatedServerBootstrapContext runtimeContext = context;
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

        DedicatedServerHeadlessWorldBootstrapResult
            worldBootstrapResult =
                BootstrapDedicatedServerHeadlessWorld(runtimeContext);
        DedicatedServerHeadlessWorldBootstrapGuard worldBootstrapGuard(
            &worldBootstrapResult.worldBootstrap);
        if (worldBootstrapResult.exitCode != 0)
        {
            return worldBootstrapResult.exitCode;
        }

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
                runtimeContext,
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
            runtimeContext.runtimeState.bindIp.c_str(),
            runtimeGuard.GetSocketState()->boundPort);

        if (options.selfConnect)
        {
            const int selfConnectExitCode = RunDedicatedServerSelfConnect(
                runtimeContext,
                *runtimeGuard.GetSocketState());
            if (selfConnectExitCode == 0)
            {
                LogInfo("shutdown", "native bootstrap self-connect ok");
            }
            return selfConnectExitCode;
        }

        if (!LceNetSetSocketNonBlocking(
                runtimeGuard.GetSocketState()->listener,
                true))
        {
            LogWarn(
                "startup",
                "failed to set native listener non-blocking; "
                "live shell accepts may stall");
        }

        NativeDedicatedServerHostedGameRuntimeStubInitData initData = {};
        DedicatedServerHeadlessRunHooksContext shellHooksContext = {};
        shellHooksContext.bootstrapContext = &runtimeContext;
        shellHooksContext.platformState = &platformState;
        shellHooksContext.options = &options;
        shellHooksContext.listener = runtimeGuard.GetSocketState()->listener;
        shellHooksContext.listenerPort =
            runtimeGuard.GetSocketState()->boundPort;

        DedicatedServerRunHooks runHooks = {};
        runHooks.afterStartupProc = &StartDedicatedServerHeadlessShellHook;
        runHooks.afterStartupContext = &shellHooksContext;
        runHooks.beforeSessionProc =
            &RunDedicatedServerHeadlessShellCommandsHook;
        runHooks.beforeSessionContext = &shellHooksContext;
        runHooks.pollProc = &PollDedicatedServerHeadlessShellHook;
        runHooks.pollContext = &shellHooksContext;

        const DedicatedServerRunExecutionResult runExecution =
            ExecuteDedicatedServerRun(
                runtimeContext.config,
                &runtimeContext.serverProperties,
                worldBootstrapResult.worldBootstrap,
                0,
                static_cast<std::int64_t>(LceGetMonotonicNanoseconds()),
                GetDedicatedServerHostedGameRuntimeThreadProc(),
                &initData,
                runHooks,
                LceGetMonotonicMilliseconds(),
                50);
        if (runExecution.abortedStartup)
        {
            if (runExecution.startup.hostedGameStartup.startupPlan
                    .shouldAbortStartup)
            {
                LogErrorf(
                    "startup",
                    "Failed to start native dedicated server (code %d).",
                    runExecution.startup.hostedGameStartup.startupResult);
            }
            return runExecution.abortExitCode;
        }

        ServerRuntime::DedicatedServerHostedGameRuntimeSessionSummary
            sessionSummary = {};
        const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
            finalSessionSnapshot =
                ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
        sessionSummary.initialSaveRequested =
            runExecution.session.initialSave.requested;
        sessionSummary.initialSaveCompleted =
            runExecution.session.initialSave.completed;
        sessionSummary.initialSaveTimedOut =
            runExecution.session.initialSave.timedOut;
        sessionSummary.sessionCompleted = runExecution.completedSession;
        sessionSummary.requestedAppShutdown =
            runExecution.session.gameplayLoop.requestedAppShutdown;
        sessionSummary.shutdownHaltedGameplay =
            runExecution.session.shutdown.haltedGameplay;
        sessionSummary.gameplayLoopIterations =
            finalSessionSnapshot.gameplayLoopIterations >
                    runExecution.session.gameplayLoop.iterations
                ? finalSessionSnapshot.gameplayLoopIterations
                : runExecution.session.gameplayLoop.iterations;
        ServerRuntime::FinalizeNativeDedicatedServerHostedGameSession(
            sessionSummary.initialSaveRequested,
            sessionSummary.initialSaveCompleted,
            sessionSummary.initialSaveTimedOut,
            sessionSummary.sessionCompleted,
            sessionSummary.requestedAppShutdown,
            sessionSummary.shutdownHaltedGameplay,
            sessionSummary.gameplayLoopIterations,
            ServerRuntime::IsDedicatedServerAppShutdownRequested(),
            ServerRuntime::IsDedicatedServerGameplayHalted(),
            ServerRuntime::IsDedicatedServerStopSignalValid(),
            LceGetMonotonicMilliseconds());
        ServerRuntime::ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
            LceGetMonotonicMilliseconds());
        PersistDedicatedServerHeadlessSaveStub(&shellHooksContext, true);

        if (!ValidateDedicatedServerHeadlessShellRun(
                options,
                shellHooksContext))
        {
            if (shellHooksContext.failureExitCode != 0)
            {
                return shellHooksContext.failureExitCode;
            }

            return 10;
        }

        LogInfo("shutdown", "native bootstrap shell stopped");
        return 0;
    }
}
