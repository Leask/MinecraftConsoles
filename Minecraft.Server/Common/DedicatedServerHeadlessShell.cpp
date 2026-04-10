#include "DedicatedServerHeadlessShell.h"

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>

#include "DedicatedServerHostedGameRuntimeState.h"
#include "Portability/NativeDedicatedServerHostedGameSession.h"
#include "DedicatedServerPlatformRuntime.h"
#include "../ServerLogger.h"
#include "DedicatedServerSignalState.h"
#include "StringUtils.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    bool IsNativeDedicatedServerHostedGameSessionRunning();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionSaveCommand(
        std::uint64_t nowMs = 0);
    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionStopCommand(
        std::uint64_t nowMs = 0);
    void ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionActivityAndProject(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle,
        std::uint64_t nowMs = 0);
}

namespace
{
    const char *GetNativeDedicatedServerHostedGameWorkerCommandKindName(
        ServerRuntime::ENativeDedicatedServerHostedGameWorkerCommandKind kind)
    {
        switch (kind)
        {
        case ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Autosave:
            return "autosave";
        case ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Save:
            return "save";
        case ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Stop:
            return "stop";
        case ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Halt:
            return "halt";
        default:
            return "none";
        }
    }

    std::string TrimAscii(const std::string &value)
    {
        size_t start = 0;
        while (start < value.size() &&
            std::isspace((unsigned char)value[start]))
        {
            ++start;
        }

        size_t end = value.size();
        while (end > start &&
            std::isspace((unsigned char)value[end - 1]))
        {
            --end;
        }

        return value.substr(start, end - start);
    }

    std::string ToLowerAscii(std::string value)
    {
        for (size_t i = 0; i < value.size(); ++i)
        {
            value[i] = (char)std::tolower((unsigned char)value[i]);
        }
        return value;
    }

    void AppendResponseLine(
        std::string *response,
        const std::string &line)
    {
        if (response == nullptr)
        {
            return;
        }

        response->append(line);
        response->push_back('\n');
    }

    bool RefreshNativeDedicatedServerActivityProjection(
        const ServerRuntime::DedicatedServerHeadlessShellState &state,
        std::uint64_t nowMs = 0)
    {
        const bool worldActionIdle =
            ServerRuntime::IsDedicatedServerWorldActionIdle(0);
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionActivityAndProject(
            state.acceptedConnections,
            state.remoteCommands,
            worldActionIdle,
            nowMs != 0 ? nowMs : LceGetMonotonicMilliseconds());
        return worldActionIdle;
    }

    void AppendStatusResponseLine(
        std::string *response,
        const ServerRuntime::DedicatedServerHeadlessShellContext &context,
        const ServerRuntime::DedicatedServerHeadlessShellState &state)
    {
        if (response == nullptr)
        {
            return;
        }

        ServerRuntime::ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot();
        const bool worldActionIdle =
            RefreshNativeDedicatedServerActivityProjection(
                state,
                LceGetMonotonicMilliseconds());
        const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot =
                ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
        const char *hostName = sessionSnapshot.context.hostName.empty()
            ? context.hostName.c_str()
            : sessionSnapshot.context.hostName.c_str();
        const char *bindIp = sessionSnapshot.context.bindIp.empty()
            ? context.bindIp.c_str()
            : sessionSnapshot.context.bindIp.c_str();
        const int configuredPort =
            sessionSnapshot.context.configuredPort > 0
            ? sessionSnapshot.context.configuredPort
            : context.multiplayerPort;
        const int listenerPort =
            sessionSnapshot.context.listenerPort > 0
            ? sessionSnapshot.context.listenerPort
            : context.listenerPort;
        const char *worldName = sessionSnapshot.context.worldName.empty()
            ? context.worldName.c_str()
            : sessionSnapshot.context.worldName.c_str();
        const char *worldSaveId =
            sessionSnapshot.context.worldSaveId.empty()
            ? context.worldSaveId.c_str()
            : sessionSnapshot.context.worldSaveId.c_str();
        char buffer[512] = {};
        std::snprintf(
            buffer,
            sizeof(buffer),
            "status host=%s bind=%s configured-port=%d listener-port=%d "
            "accepted=%llu world-action=%s",
            hostName,
            bindIp,
            configuredPort,
            listenerPort,
            (unsigned long long)
                sessionSnapshot.progress.acceptedConnections,
            worldActionIdle ? "idle" : "busy");
        AppendResponseLine(response, buffer);

        std::snprintf(
            buffer,
            sizeof(buffer),
            "status world=%s level-id=%s storage=%s whitelist=%s lan=%s",
            worldName,
            worldSaveId,
            context.storageRoot.c_str(),
            context.whitelistEnabled ? "enabled" : "disabled",
            context.lanAdvertise ? "enabled" : "disabled");
        AppendResponseLine(response, buffer);

        if (sessionSnapshot.startAttempted)
        {
            std::snprintf(
                buffer,
                sizeof(buffer),
                "status runtime=%s seed=%lld world-size=%u hell-scale=%u "
                "public-slots=%u private-slots=%u startup=%d thread=%s "
                "phase=%s",
                sessionSnapshot.loadedFromSave ? "loaded" : "created-new",
                (long long)sessionSnapshot.resolvedSeed,
                sessionSnapshot.worldConfig.worldSizeChunks,
                (unsigned int)sessionSnapshot.worldConfig.worldHellScale,
                (unsigned int)sessionSnapshot.activation.publicSlots,
                (unsigned int)sessionSnapshot.activation.privateSlots,
                sessionSnapshot.startup.result,
                sessionSnapshot.threadInvoked ? "invoked" : "skipped",
                ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
                    (ServerRuntime::EDedicatedServerHostedGameRuntimePhase)
                        sessionSnapshot.runtimePhase));
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
                "status payload settings=0x%08x no-local=%s "
                "checksum=0x%016llx save-generation=%llu "
                "state-checksum=0x%016llx startup-payload=%s "
                "validated=%s startup-steps=%llu startup-ms=%llu",
                (unsigned int)sessionSnapshot.worldConfig.hostSettings,
                sessionSnapshot.worldConfig.dedicatedNoLocalHostPlayer
                    ? "true"
                    : "false",
                (unsigned long long)sessionSnapshot.payload.checksum,
                (unsigned long long)sessionSnapshot.progress.saveGeneration,
                (unsigned long long)sessionSnapshot.progress.stateChecksum,
                sessionSnapshot.payload.bytes > 0 ? "present" : "none",
                sessionSnapshot.startup.payloadValidated
                    ? "true"
                    : "false",
                (unsigned long long)
                    sessionSnapshot.startup.threadIterations,
                (unsigned long long)
                    sessionSnapshot.startup.threadDurationMs);
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
                "status session active=%s world=%s level-id=%s payload=%s "
                "payload-bytes=%lld autosaves=%llu/%llu ticks=%llu "
                "uptime-ms=%llu action=%s shutdown=%s halted=%s "
                "hosted-thread=%s thread-ticks=%llu",
                sessionSnapshot.active ? "true" : "false",
                worldName,
                worldSaveId,
                sessionSnapshot.payload.name.empty()
                    ? "none"
                    : sessionSnapshot.payload.name.c_str(),
                (long long)sessionSnapshot.payload.bytes,
                (unsigned long long)sessionSnapshot.progress.autosaveRequests,
                (unsigned long long)
                    sessionSnapshot.progress.observedAutosaveCompletions,
                (unsigned long long)
                    sessionSnapshot.progress.platformTickCount,
                (unsigned long long)
                    (sessionSnapshot.timing.sessionStartMs == 0
                        ? 0
                        : ((sessionSnapshot.timing.stoppedMs != 0
                                ? sessionSnapshot.timing.stoppedMs
                                : LceGetMonotonicMilliseconds()) -
                            sessionSnapshot.timing.sessionStartMs)),
                sessionSnapshot.control.worldActionIdle ? "idle" : "busy",
                sessionSnapshot.control.appShutdownRequested
                    ? "true"
                    : "false",
                sessionSnapshot.control.gameplayHalted ? "true" : "false",
                sessionSnapshot.thread.active ? "active" : "stopped",
                (unsigned long long)sessionSnapshot.thread.ticks);
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
                "status worker pending=%llu autoq=%llu saveq=%llu stopq=%llu haltq=%llu "
                "ticks=%llu completed=%llu auto-ops=%llu save-ops=%llu "
                "stop-ops=%llu halt-ops=%llu active=%s#%llu/%llu "
                "last-queued=%llu last-processed=%s#%llu "
                "core-checksum=0x%016llx",
                (unsigned long long)
                    sessionSnapshot.worker.pendingWorldActionTicks,
                (unsigned long long)
                    sessionSnapshot.worker.pendingAutosaveCommands,
                (unsigned long long)
                    sessionSnapshot.worker.pendingSaveCommands,
                (unsigned long long)
                    sessionSnapshot.worker.pendingStopCommands,
                (unsigned long long)
                    sessionSnapshot.worker.pendingHaltCommands,
                (unsigned long long)
                    sessionSnapshot.worker.workerTickCount,
                (unsigned long long)
                    sessionSnapshot.worker.completedWorldActions,
                (unsigned long long)
                    sessionSnapshot.worker.processedAutosaveCommands,
                (unsigned long long)
                    sessionSnapshot.worker.processedSaveCommands,
                (unsigned long long)
                    sessionSnapshot.worker.processedStopCommands,
                (unsigned long long)
                    sessionSnapshot.worker.processedHaltCommands,
                GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                    (ServerRuntime::
                        ENativeDedicatedServerHostedGameWorkerCommandKind)
                        sessionSnapshot.worker.activeCommandKind),
                (unsigned long long)
                    sessionSnapshot.worker.activeCommandId,
                (unsigned long long)
                    sessionSnapshot.worker.activeCommandTicksRemaining,
                (unsigned long long)
                    sessionSnapshot.worker.lastQueuedCommandId,
                GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                    (ServerRuntime::
                        ENativeDedicatedServerHostedGameWorkerCommandKind)
                        sessionSnapshot.worker.lastProcessedCommandKind),
                (unsigned long long)
                    sessionSnapshot.worker.lastProcessedCommandId,
                (unsigned long long)sessionSnapshot.progress.stateChecksum);
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
                "status run initial-save=%s/%s/%s session-completed=%s "
                "app-shutdown=%s shutdown-halted=%s gameplay-iterations=%llu",
                sessionSnapshot.summary.initialSaveRequested
                    ? "requested"
                    : "skipped",
                sessionSnapshot.summary.initialSaveCompleted
                    ? "completed"
                    : "pending",
                sessionSnapshot.summary.initialSaveTimedOut
                    ? "timed-out"
                    : "ok",
                sessionSnapshot.summary.sessionCompleted ? "true" : "false",
                sessionSnapshot.summary.requestedAppShutdown
                    ? "true"
                    : "false",
                sessionSnapshot.summary.shutdownHaltedGameplay
                    ? "true"
                    : "false",
                (unsigned long long)
                    sessionSnapshot.progress.gameplayLoopIterations);
            AppendResponseLine(response, buffer);

            if (!sessionSnapshot.context.savePath.empty() ||
                !sessionSnapshot.persistedSave.savePath.empty())
            {
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "status save path=%s last-persisted=%s "
                    "saved-at-filetime=%llu persisted-autosaves=%llu",
                    sessionSnapshot.context.savePath.c_str(),
                    sessionSnapshot.persistedSave.savePath.c_str(),
                    (unsigned long long)
                        sessionSnapshot.persistedSave.fileTime,
                    (unsigned long long)
                        sessionSnapshot.persistedSave.autosaveCompletions);
                AppendResponseLine(response, buffer);
            }

            const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
                runtimeSnapshot =
                    ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();

            if (runtimeSnapshot.loadedSaveMetadataAvailable)
            {
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "status loaded-save path=%s startup=%s phase=%s "
                    "remote=%llu autosaves=%llu ticks=%llu uptime-ms=%llu "
                    "worker-pending=%llu autoq=%llu saveq=%llu stopq=%llu haltq=%llu "
                    "worker-ticks=%llu worker-completions=%llu "
                    "auto-ops=%llu save-ops=%llu stop-ops=%llu halt-ops=%llu "
                    "active=%s#%llu/%llu last-queued=%llu "
                    "last-processed=%s#%llu completed=%s "
                    "app-shutdown=%s shutdown-halted=%s "
                    "gameplay-iterations=%llu hosted-thread=%s "
                    "thread-ticks=%llu",
                    runtimeSnapshot.loadedSavePath.c_str(),
                    runtimeSnapshot.previousStartupMode.c_str(),
                    runtimeSnapshot.previousSessionPhase.c_str(),
                    (unsigned long long)
                        runtimeSnapshot.previousRemoteCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousAutosaveCompletions,
                    (unsigned long long)
                        runtimeSnapshot.previousPlatformTickCount,
                    (unsigned long long)runtimeSnapshot.previousUptimeMs,
                    (unsigned long long)
                        runtimeSnapshot
                            .previousWorkerPendingWorldActionTicks,
                    (unsigned long long)
                        runtimeSnapshot.previousWorkerPendingAutosaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousWorkerPendingSaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousWorkerPendingStopCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousWorkerPendingHaltCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousWorkerTickCount,
                    (unsigned long long)
                        runtimeSnapshot.previousCompletedWorkerActions,
                    (unsigned long long)
                        runtimeSnapshot.previousProcessedAutosaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousProcessedSaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousProcessedStopCommands,
                    (unsigned long long)
                        runtimeSnapshot.previousProcessedHaltCommands,
                    GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                        (ServerRuntime::
                            ENativeDedicatedServerHostedGameWorkerCommandKind)
                            runtimeSnapshot.previousActiveCommandKind),
                    (unsigned long long)
                        runtimeSnapshot.previousActiveCommandId,
                    (unsigned long long)
                        runtimeSnapshot.previousActiveCommandTicksRemaining,
                    (unsigned long long)
                        runtimeSnapshot.previousLastQueuedCommandId,
                    GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                        (ServerRuntime::
                            ENativeDedicatedServerHostedGameWorkerCommandKind)
                            runtimeSnapshot.previousLastProcessedCommandKind),
                    (unsigned long long)
                        runtimeSnapshot.previousLastProcessedCommandId,
                    runtimeSnapshot.previousSessionCompleted ? "true" : "false",
                    runtimeSnapshot.previousRequestedAppShutdown
                        ? "true"
                        : "false",
                    runtimeSnapshot.previousShutdownHaltedGameplay
                        ? "true"
                        : "false",
                    (unsigned long long)
                        runtimeSnapshot.previousGameplayLoopIterations,
                    runtimeSnapshot.previousHostedThreadActive
                        ? "active"
                        : "stopped",
                    (unsigned long long)
                        runtimeSnapshot.previousHostedThreadTicks);
                AppendResponseLine(response, buffer);

                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "status loaded-payload settings=0x%08x no-local=%s "
                    "world-size=%u hell-scale=%u online=%s private=%s "
                    "fake-local=%s public-slots=%u private-slots=%u "
                    "checksum=0x%016llx save-generation=%llu "
                    "state-checksum=0x%016llx startup-payload=%s "
                    "validated=%s startup-steps=%llu startup-ms=%llu",
                    (unsigned int)runtimeSnapshot.previousHostSettings,
                    runtimeSnapshot.previousDedicatedNoLocalHostPlayer
                        ? "true"
                        : "false",
                    runtimeSnapshot.previousWorldSizeChunks,
                    (unsigned int)runtimeSnapshot.previousWorldHellScale,
                    runtimeSnapshot.previousOnlineGame ? "true" : "false",
                    runtimeSnapshot.previousPrivateGame ? "true" : "false",
                    runtimeSnapshot.previousFakeLocalPlayerJoined
                        ? "true"
                        : "false",
                    (unsigned int)runtimeSnapshot.previousPublicSlots,
                    (unsigned int)runtimeSnapshot.previousPrivateSlots,
                    (unsigned long long)
                        runtimeSnapshot.previousSavePayloadChecksum,
                    (unsigned long long)
                        runtimeSnapshot.previousSaveGeneration,
                    (unsigned long long)
                        runtimeSnapshot.previousSessionStateChecksum,
                    runtimeSnapshot.previousStartupPayloadPresent
                        ? "present"
                        : "none",
                    runtimeSnapshot.previousStartupPayloadValidated
                        ? "true"
                        : "false",
                    (unsigned long long)
                        runtimeSnapshot.previousStartupThreadIterations,
                    (unsigned long long)
                        runtimeSnapshot.previousStartupThreadDurationMs);
                AppendResponseLine(response, buffer);
            }
        }
    }

    std::vector<std::string> SplitShellRequestLines(
        const std::string &request)
    {
        std::vector<std::string> lines;
        size_t start = 0;
        while (start < request.size())
        {
            const size_t end = request.find('\n', start);
            std::string line = end == std::string::npos
                ? request.substr(start)
                : request.substr(start, end - start);
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            lines.push_back(line);
            if (end == std::string::npos)
            {
                break;
            }

            start = end + 1;
        }

        return lines;
    }
}

namespace ServerRuntime
{
    DedicatedServerHeadlessShellContext
    BuildDedicatedServerHeadlessShellContext(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        int listenerPort)
    {
        DedicatedServerHeadlessShellContext shellContext = {};
        shellContext.storageRoot = context.storageRoot;
        shellContext.worldName = StringUtils::WideToUtf8(
            context.serverProperties.worldName);
        shellContext.worldSaveId = context.serverProperties.worldSaveId;
        shellContext.bindIp = platformState.bindIp;
        shellContext.hostName = platformState.userNameUtf8;
        shellContext.multiplayerPort = platformState.multiplayerPort;
        shellContext.listenerPort = listenerPort;
        shellContext.whitelistEnabled =
            context.serverProperties.whiteListEnabled;
        shellContext.lanAdvertise = context.serverProperties.lanAdvertise;
        return shellContext;
    }

    void LogDedicatedServerHeadlessShellHelp()
    {
        LogInfo("console", "native shell commands: help, save, status, stop");
    }

    void PollDedicatedServerHeadlessShellConnections(
        LceSocketHandle listener,
        const DedicatedServerHeadlessShellContext &context,
        DedicatedServerHeadlessShellState *state)
    {
        if (listener == LCE_INVALID_SOCKET || state == nullptr)
        {
            return;
        }

        for (;;)
        {
            char acceptedIp[64] = {};
            int acceptedPort = -1;
            const LceSocketHandle acceptedSocket = LceNetAcceptIpv4(
                listener,
                acceptedIp,
                sizeof(acceptedIp),
                &acceptedPort);
            if (acceptedSocket == LCE_INVALID_SOCKET)
            {
                break;
            }

            ++state->acceptedConnections;
            RefreshNativeDedicatedServerActivityProjection(*state);
            LogInfof(
                "network",
                "native shell accepted connection #%llu from %s:%d",
                (unsigned long long)state->acceptedConnections,
                acceptedIp,
                acceptedPort);

            std::string response;
            LceNetSetSocketRecvTimeout(acceptedSocket, 25);
            std::string request;
            char buffer[256] = {};
            for (;;)
            {
                const int received = LceNetRecv(
                    acceptedSocket,
                    buffer,
                    sizeof(buffer));
                if (received <= 0)
                {
                    break;
                }

                request.append(buffer, (size_t)received);
                if (request.size() >= 2048)
                {
                    break;
                }
            }

            const std::vector<std::string> requestLines =
                SplitShellRequestLines(request);
            if (requestLines.empty())
            {
                AppendResponseLine(
                    &response,
                    "ok connected; send newline-delimited commands");
            }

            for (size_t i = 0; i < requestLines.size(); ++i)
            {
                const std::string trimmedLine = TrimAscii(requestLines[i]);
                if (trimmedLine.empty())
                {
                    continue;
                }

                ++state->remoteCommands;
                RefreshNativeDedicatedServerActivityProjection(*state);
                LogInfof(
                    "console",
                    "remote shell command #%llu: %s",
                    (unsigned long long)state->remoteCommands,
                    trimmedLine.c_str());
                ExecuteDedicatedServerHeadlessShellCommand(
                    trimmedLine,
                    context,
                    *state,
                    &response);
            }

            if (!response.empty())
            {
                LceNetSendAll(
                    acceptedSocket,
                    response.data(),
                    (int)response.size());
            }
            LceNetCloseSocket(acceptedSocket);
        }
    }

    bool ExecuteDedicatedServerHeadlessShellCommand(
        const std::string &line,
        const DedicatedServerHeadlessShellContext &context,
        const DedicatedServerHeadlessShellState &state,
        std::string *response)
    {
        const std::string trimmedLine = TrimAscii(line);
        if (trimmedLine.empty())
        {
            return true;
        }

        const std::string command = ToLowerAscii(trimmedLine);
        if (command == "help")
        {
            LogDedicatedServerHeadlessShellHelp();
            AppendResponseLine(
                response,
                "commands: help, save, status, stop");
            return true;
        }

        if (command == "status")
        {
            ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
                LceGetMonotonicMilliseconds());
            const bool worldActionIdle =
                RefreshNativeDedicatedServerActivityProjection(
                    state,
                    LceGetMonotonicMilliseconds());
            LogInfof(
                "console",
                "status host=%s bind=%s configured-port=%d listener-port=%d accepted=%llu world-action=%s",
                context.hostName.c_str(),
                context.bindIp.c_str(),
                context.multiplayerPort,
                context.listenerPort,
                (unsigned long long)state.acceptedConnections,
                worldActionIdle ? "idle" : "busy");
            LogInfof(
                "console",
                "status world=%s level-id=%s storage=%s whitelist=%s lan=%s",
                context.worldName.c_str(),
                context.worldSaveId.c_str(),
                context.storageRoot.c_str(),
                context.whitelistEnabled ? "enabled" : "disabled",
                context.lanAdvertise ? "enabled" : "disabled");
            const DedicatedServerHostedGameRuntimeSnapshot runtimeSnapshot =
                GetDedicatedServerHostedGameRuntimeSnapshot();
            if (runtimeSnapshot.startAttempted)
            {
                LogInfof(
                    "console",
                    "status runtime=%s seed=%lld world-size=%u hell-scale=%u "
                    "public-slots=%u private-slots=%u startup=%d thread=%s "
                    "phase=%s",
                    runtimeSnapshot.loadedFromSave ? "loaded" : "created-new",
                    (long long)runtimeSnapshot.resolvedSeed,
                    runtimeSnapshot.worldSizeChunks,
                    (unsigned int)runtimeSnapshot.worldHellScale,
                    (unsigned int)runtimeSnapshot.publicSlots,
                    (unsigned int)runtimeSnapshot.privateSlots,
                    runtimeSnapshot.startupResult,
                    runtimeSnapshot.threadInvoked ? "invoked" : "skipped",
                    GetDedicatedServerHostedGameRuntimePhaseName(
                        runtimeSnapshot.phase));
                LogInfof(
                    "console",
                    "status payload settings=0x%08x no-local=%s "
                    "checksum=0x%016llx save-generation=%llu "
                    "state-checksum=0x%016llx "
                    "startup-payload=%s "
                    "validated=%s startup-steps=%llu startup-ms=%llu",
                    (unsigned int)runtimeSnapshot.hostSettings,
                    runtimeSnapshot.dedicatedNoLocalHostPlayer
                        ? "true"
                        : "false",
                    (unsigned long long)
                        runtimeSnapshot.savePayloadChecksum,
                    (unsigned long long)
                        runtimeSnapshot.saveGeneration,
                    (unsigned long long)
                        runtimeSnapshot.sessionStateChecksum,
                    runtimeSnapshot.startupPayloadPresent
                        ? "present"
                        : "none",
                    runtimeSnapshot.startupPayloadValidated
                        ? "true"
                        : "false",
                    (unsigned long long)
                        runtimeSnapshot.startupThreadIterations,
                    (unsigned long long)
                        runtimeSnapshot.startupThreadDurationMs);
                LogInfof(
                    "console",
                    "status session active=%s world=%s level-id=%s payload=%s "
                    "payload-bytes=%lld autosaves=%llu/%llu ticks=%llu "
                    "uptime-ms=%llu action=%s shutdown=%s halted=%s "
                    "hosted-thread=%s thread-ticks=%llu",
                    runtimeSnapshot.sessionActive ? "true" : "false",
                    runtimeSnapshot.worldName.c_str(),
                    runtimeSnapshot.worldSaveId.c_str(),
                    runtimeSnapshot.savePayloadName.empty()
                        ? "none"
                        : runtimeSnapshot.savePayloadName.c_str(),
                    (long long)runtimeSnapshot.savePayloadBytes,
                    (unsigned long long)runtimeSnapshot.autosaveRequests,
                    (unsigned long long)runtimeSnapshot.autosaveCompletions,
                    (unsigned long long)runtimeSnapshot.platformTickCount,
                    (unsigned long long)runtimeSnapshot.uptimeMs,
                    runtimeSnapshot.worldActionIdle ? "idle" : "busy",
                    runtimeSnapshot.appShutdownRequested ? "true" : "false",
                    runtimeSnapshot.gameplayHalted ? "true" : "false",
                    runtimeSnapshot.hostedThreadActive
                        ? "active"
                        : "stopped",
                    (unsigned long long)runtimeSnapshot.hostedThreadTicks);

                LogInfof(
                    "console",
                    "status worker pending=%llu autoq=%llu saveq=%llu stopq=%llu haltq=%llu "
                    "ticks=%llu completed=%llu auto-ops=%llu save-ops=%llu "
                    "stop-ops=%llu halt-ops=%llu active=%s#%llu/%llu "
                    "last-queued=%llu last-processed=%s#%llu "
                    "core-checksum=0x%016llx",
                    (unsigned long long)
                        runtimeSnapshot.workerPendingWorldActionTicks,
                    (unsigned long long)
                        runtimeSnapshot.workerPendingAutosaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.workerPendingSaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.workerPendingStopCommands,
                    (unsigned long long)
                        runtimeSnapshot.workerPendingHaltCommands,
                    (unsigned long long)
                        runtimeSnapshot.workerTickCount,
                    (unsigned long long)
                        runtimeSnapshot.completedWorkerActions,
                    (unsigned long long)
                        runtimeSnapshot.processedAutosaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.processedSaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.processedStopCommands,
                    (unsigned long long)
                        runtimeSnapshot.processedHaltCommands,
                    GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                        (ServerRuntime::
                            ENativeDedicatedServerHostedGameWorkerCommandKind)
                            runtimeSnapshot.activeCommandKind),
                    (unsigned long long)
                        runtimeSnapshot.activeCommandId,
                    (unsigned long long)
                        runtimeSnapshot.activeCommandTicksRemaining,
                    (unsigned long long)
                        runtimeSnapshot.lastQueuedCommandId,
                    GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                        (ServerRuntime::
                            ENativeDedicatedServerHostedGameWorkerCommandKind)
                            runtimeSnapshot.lastProcessedCommandKind),
                    (unsigned long long)
                        runtimeSnapshot.lastProcessedCommandId,
                    (unsigned long long)
                        runtimeSnapshot.sessionStateChecksum);

                LogInfof(
                    "console",
                    "status run initial-save=%s/%s/%s session-completed=%s "
                    "app-shutdown=%s shutdown-halted=%s gameplay-iterations=%llu",
                    runtimeSnapshot.initialSaveRequested
                        ? "requested"
                        : "skipped",
                    runtimeSnapshot.initialSaveCompleted
                        ? "completed"
                        : "pending",
                    runtimeSnapshot.initialSaveTimedOut
                        ? "timed-out"
                        : "ok",
                    runtimeSnapshot.sessionCompleted ? "true" : "false",
                    runtimeSnapshot.requestedAppShutdown ? "true" : "false",
                    runtimeSnapshot.shutdownHaltedGameplay ? "true" : "false",
                    (unsigned long long)
                        runtimeSnapshot.gameplayLoopIterations);

                if (!runtimeSnapshot.savePath.empty() ||
                    !runtimeSnapshot.lastPersistedSavePath.empty())
                {
                    LogInfof(
                        "console",
                        "status save path=%s last-persisted=%s "
                        "saved-at-filetime=%llu persisted-autosaves=%llu",
                        runtimeSnapshot.savePath.c_str(),
                        runtimeSnapshot.lastPersistedSavePath.c_str(),
                        (unsigned long long)
                            runtimeSnapshot.lastPersistedFileTime,
                        (unsigned long long)
                            runtimeSnapshot.lastPersistedAutosaveCompletions);
                }

                if (runtimeSnapshot.loadedSaveMetadataAvailable)
                {
                    LogInfof(
                        "console",
                        "status loaded-save path=%s startup=%s phase=%s "
                        "remote=%llu autosaves=%llu ticks=%llu uptime-ms=%llu "
                        "worker-pending=%llu autoq=%llu saveq=%llu stopq=%llu haltq=%llu "
                        "worker-ticks=%llu worker-completions=%llu "
                        "auto-ops=%llu save-ops=%llu stop-ops=%llu halt-ops=%llu "
                        "active=%s#%llu/%llu last-queued=%llu "
                        "last-processed=%s#%llu completed=%s "
                        "app-shutdown=%s shutdown-halted=%s "
                        "gameplay-iterations=%llu hosted-thread=%s "
                        "thread-ticks=%llu",
                        runtimeSnapshot.loadedSavePath.c_str(),
                        runtimeSnapshot.previousStartupMode.c_str(),
                        runtimeSnapshot.previousSessionPhase.c_str(),
                        (unsigned long long)
                            runtimeSnapshot.previousRemoteCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousAutosaveCompletions,
                        (unsigned long long)
                            runtimeSnapshot.previousPlatformTickCount,
                        (unsigned long long)
                            runtimeSnapshot.previousUptimeMs,
                        (unsigned long long)
                            runtimeSnapshot
                                .previousWorkerPendingWorldActionTicks,
                        (unsigned long long)
                            runtimeSnapshot
                                .previousWorkerPendingAutosaveCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousWorkerPendingSaveCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousWorkerPendingStopCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousWorkerPendingHaltCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousWorkerTickCount,
                        (unsigned long long)
                            runtimeSnapshot.previousCompletedWorkerActions,
                        (unsigned long long)
                            runtimeSnapshot.previousProcessedAutosaveCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousProcessedSaveCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousProcessedStopCommands,
                        (unsigned long long)
                            runtimeSnapshot.previousProcessedHaltCommands,
                        GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                            (ServerRuntime::
                                ENativeDedicatedServerHostedGameWorkerCommandKind)
                                runtimeSnapshot.previousActiveCommandKind),
                        (unsigned long long)
                            runtimeSnapshot.previousActiveCommandId,
                        (unsigned long long)
                            runtimeSnapshot.previousActiveCommandTicksRemaining,
                        (unsigned long long)
                            runtimeSnapshot.previousLastQueuedCommandId,
                        GetNativeDedicatedServerHostedGameWorkerCommandKindName(
                            (ServerRuntime::
                                ENativeDedicatedServerHostedGameWorkerCommandKind)
                                runtimeSnapshot
                                    .previousLastProcessedCommandKind),
                        (unsigned long long)
                            runtimeSnapshot.previousLastProcessedCommandId,
                        runtimeSnapshot.previousSessionCompleted
                            ? "true"
                            : "false",
                        runtimeSnapshot.previousRequestedAppShutdown
                            ? "true"
                            : "false",
                        runtimeSnapshot.previousShutdownHaltedGameplay
                            ? "true"
                            : "false",
                        (unsigned long long)
                            runtimeSnapshot.previousGameplayLoopIterations,
                        runtimeSnapshot.previousHostedThreadActive
                            ? "active"
                            : "stopped",
                        (unsigned long long)
                            runtimeSnapshot.previousHostedThreadTicks);
                    LogInfof(
                        "console",
                        "status loaded-payload settings=0x%08x no-local=%s "
                        "world-size=%u hell-scale=%u online=%s private=%s "
                        "fake-local=%s public-slots=%u private-slots=%u "
                        "checksum=0x%016llx save-generation=%llu "
                        "state-checksum=0x%016llx startup-payload=%s "
                        "validated=%s startup-steps=%llu startup-ms=%llu",
                        (unsigned int)runtimeSnapshot.previousHostSettings,
                        runtimeSnapshot.previousDedicatedNoLocalHostPlayer
                            ? "true"
                            : "false",
                        runtimeSnapshot.previousWorldSizeChunks,
                        (unsigned int)
                            runtimeSnapshot.previousWorldHellScale,
                        runtimeSnapshot.previousOnlineGame ? "true" : "false",
                        runtimeSnapshot.previousPrivateGame ? "true" : "false",
                        runtimeSnapshot.previousFakeLocalPlayerJoined
                            ? "true"
                            : "false",
                        (unsigned int)runtimeSnapshot.previousPublicSlots,
                        (unsigned int)runtimeSnapshot.previousPrivateSlots,
                        (unsigned long long)
                            runtimeSnapshot.previousSavePayloadChecksum,
                        (unsigned long long)
                            runtimeSnapshot.previousSaveGeneration,
                        (unsigned long long)
                            runtimeSnapshot.previousSessionStateChecksum,
                        runtimeSnapshot.previousStartupPayloadPresent
                            ? "present"
                            : "none",
                        runtimeSnapshot.previousStartupPayloadValidated
                            ? "true"
                            : "false",
                        (unsigned long long)
                            runtimeSnapshot.previousStartupThreadIterations,
                        (unsigned long long)
                            runtimeSnapshot.previousStartupThreadDurationMs);
                }
            }
            AppendStatusResponseLine(response, context, state);
            return true;
        }

        if (command == "save")
        {
            std::uint64_t commandId = 0;
            if (IsNativeDedicatedServerHostedGameSessionRunning())
            {
                commandId =
                    EnqueueNativeDedicatedServerHostedGameSessionSaveCommand(
                        LceGetMonotonicMilliseconds());
            }
            else
            {
                RequestDedicatedServerWorldAutosave(0);
            }
            if (commandId != 0)
            {
                LogInfof(
                    "console",
                    "manual save requested via worker command #%llu",
                    (unsigned long long)commandId);
                char buffer[128] = {};
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "ok manual save requested command=%llu",
                    (unsigned long long)commandId);
                AppendResponseLine(response, buffer);
            }
            else
            {
                LogInfo("console", "manual save requested");
                AppendResponseLine(response, "ok manual save requested");
            }
            return true;
        }

        if (command == "stop")
        {
            std::uint64_t commandId = 0;
            if (IsNativeDedicatedServerHostedGameSessionRunning())
            {
                commandId =
                    EnqueueNativeDedicatedServerHostedGameSessionStopCommand(
                        LceGetMonotonicMilliseconds());
            }
            else
            {
                RequestDedicatedServerShutdown();
            }
            if (commandId != 0)
            {
                LogInfof(
                    "console",
                    "stop requested via worker command #%llu",
                    (unsigned long long)commandId);
                char buffer[128] = {};
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "ok stop requested command=%llu",
                    (unsigned long long)commandId);
                AppendResponseLine(response, buffer);
            }
            else
            {
                LogInfo("console", "stop requested");
                AppendResponseLine(response, "ok stop requested");
            }
            return true;
        }

        LogWarnf(
            "console",
            "Unknown native shell command: %s",
            trimmedLine.c_str());
        AppendResponseLine(response, "error unknown command");
        return false;
    }
}
