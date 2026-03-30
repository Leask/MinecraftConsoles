#include "DedicatedServerHeadlessShell.h"

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>

#include "DedicatedServerHostedGameRuntimeState.h"
#include "Portability/NativeDedicatedServerHostedGameSession.h"
#include "Portability/NativeDedicatedServerHostedGameWorker.h"
#include "DedicatedServerPlatformRuntime.h"
#include "../ServerLogger.h"
#include "DedicatedServerSignalState.h"
#include "StringUtils.h"
#include "lce_time/lce_time.h"

namespace
{
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

    void AppendStatusResponseLine(
        std::string *response,
        const ServerRuntime::DedicatedServerHeadlessShellContext &context,
        const ServerRuntime::DedicatedServerHeadlessShellState &state)
    {
        if (response == nullptr)
        {
            return;
        }

        const bool worldActionIdle =
            ServerRuntime::IsDedicatedServerWorldActionIdle(0);
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionActivity(
            state.acceptedConnections,
            state.remoteCommands,
            worldActionIdle);
        ServerRuntime::UpdateDedicatedServerHostedGameRuntimeSessionState(
            state.acceptedConnections,
            state.remoteCommands,
            ServerRuntime::GetDedicatedServerAutosaveRequestCount(),
            ServerRuntime::GetDedicatedServerAutosaveCompletionCount(),
            ServerRuntime::GetDedicatedServerPlatformTickCount(),
            worldActionIdle,
            ServerRuntime::IsDedicatedServerAppShutdownRequested(),
            ServerRuntime::IsDedicatedServerGameplayHalted(),
            ServerRuntime::IsDedicatedServerStopSignalValid(),
            LceGetMonotonicMilliseconds());
        char buffer[512] = {};
        std::snprintf(
            buffer,
            sizeof(buffer),
            "status host=%s bind=%s configured-port=%d listener-port=%d "
            "accepted=%llu world-action=%s",
            context.hostName.c_str(),
            context.bindIp.c_str(),
            context.multiplayerPort,
            context.listenerPort,
            (unsigned long long)state.acceptedConnections,
            worldActionIdle ? "idle" : "busy");
        AppendResponseLine(response, buffer);

        std::snprintf(
            buffer,
            sizeof(buffer),
            "status world=%s level-id=%s storage=%s whitelist=%s lan=%s",
            context.worldName.c_str(),
            context.worldSaveId.c_str(),
            context.storageRoot.c_str(),
            context.whitelistEnabled ? "enabled" : "disabled",
            context.lanAdvertise ? "enabled" : "disabled");
        AppendResponseLine(response, buffer);

        const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
            runtimeSnapshot =
                ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
        if (runtimeSnapshot.startAttempted)
        {
            std::snprintf(
                buffer,
                sizeof(buffer),
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
                ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
                    runtimeSnapshot.phase));
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
                "status payload settings=0x%08x no-local=%s "
                "checksum=0x%016llx save-generation=%llu "
                "state-checksum=0x%016llx startup-payload=%s "
                "validated=%s startup-steps=%llu startup-ms=%llu",
                (unsigned int)runtimeSnapshot.hostSettings,
                runtimeSnapshot.dedicatedNoLocalHostPlayer
                    ? "true"
                    : "false",
                (unsigned long long)runtimeSnapshot.savePayloadChecksum,
                (unsigned long long)runtimeSnapshot.saveGeneration,
                (unsigned long long)runtimeSnapshot.sessionStateChecksum,
                runtimeSnapshot.startupPayloadPresent ? "present" : "none",
                runtimeSnapshot.startupPayloadValidated ? "true" : "false",
                (unsigned long long)
                    runtimeSnapshot.startupThreadIterations,
                (unsigned long long)
                    runtimeSnapshot.startupThreadDurationMs);
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
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
                runtimeSnapshot.hostedThreadActive ? "active" : "stopped",
                (unsigned long long)runtimeSnapshot.hostedThreadTicks);
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
                "status worker pending=%llu saveq=%llu stopq=%llu "
                "ticks=%llu completed=%llu save-ops=%llu stop-ops=%llu "
                "core-checksum=0x%016llx",
                (unsigned long long)
                    runtimeSnapshot.workerPendingWorldActionTicks,
                (unsigned long long)
                    runtimeSnapshot.workerPendingSaveCommands,
                (unsigned long long)
                    runtimeSnapshot.workerPendingStopCommands,
                (unsigned long long)
                    runtimeSnapshot.workerTickCount,
                (unsigned long long)
                    runtimeSnapshot.completedWorkerActions,
                (unsigned long long)
                    runtimeSnapshot.processedSaveCommands,
                (unsigned long long)
                    runtimeSnapshot.processedStopCommands,
                (unsigned long long)runtimeSnapshot.sessionStateChecksum);
            AppendResponseLine(response, buffer);

            std::snprintf(
                buffer,
                sizeof(buffer),
                "status run initial-save=%s/%s/%s session-completed=%s "
                "app-shutdown=%s shutdown-halted=%s gameplay-iterations=%llu",
                runtimeSnapshot.initialSaveRequested ? "requested" : "skipped",
                runtimeSnapshot.initialSaveCompleted ? "completed" : "pending",
                runtimeSnapshot.initialSaveTimedOut ? "timed-out" : "ok",
                runtimeSnapshot.sessionCompleted ? "true" : "false",
                runtimeSnapshot.requestedAppShutdown ? "true" : "false",
                runtimeSnapshot.shutdownHaltedGameplay ? "true" : "false",
                (unsigned long long)runtimeSnapshot.gameplayLoopIterations);
            AppendResponseLine(response, buffer);

            if (!runtimeSnapshot.savePath.empty() ||
                !runtimeSnapshot.lastPersistedSavePath.empty())
            {
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "status save path=%s last-persisted=%s "
                    "saved-at-filetime=%llu persisted-autosaves=%llu",
                    runtimeSnapshot.savePath.c_str(),
                    runtimeSnapshot.lastPersistedSavePath.c_str(),
                    (unsigned long long)
                        runtimeSnapshot.lastPersistedFileTime,
                    (unsigned long long)
                        runtimeSnapshot.lastPersistedAutosaveCompletions);
                AppendResponseLine(response, buffer);
            }

            if (runtimeSnapshot.loadedSaveMetadataAvailable)
            {
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "status loaded-save path=%s startup=%s phase=%s "
                    "remote=%llu autosaves=%llu ticks=%llu uptime-ms=%llu "
                    "worker-pending=%llu worker-ticks=%llu "
                    "worker-completions=%llu completed=%s "
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
                        runtimeSnapshot.previousWorkerTickCount,
                    (unsigned long long)
                        runtimeSnapshot.previousCompletedWorkerActions,
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
            const bool worldActionIdle =
                IsDedicatedServerWorldActionIdle(0);
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
            UpdateDedicatedServerHostedGameRuntimeSessionState(
                state.acceptedConnections,
                state.remoteCommands,
                GetDedicatedServerAutosaveRequestCount(),
                GetDedicatedServerAutosaveCompletionCount(),
                GetDedicatedServerPlatformTickCount(),
                worldActionIdle,
                IsDedicatedServerAppShutdownRequested(),
                IsDedicatedServerGameplayHalted(),
                IsDedicatedServerStopSignalValid(),
                LceGetMonotonicMilliseconds());
            ObserveNativeDedicatedServerHostedGameSessionActivity(
                state.acceptedConnections,
                state.remoteCommands,
                worldActionIdle);
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
                    "status worker pending=%llu saveq=%llu stopq=%llu "
                    "ticks=%llu completed=%llu save-ops=%llu stop-ops=%llu "
                    "core-checksum=0x%016llx",
                    (unsigned long long)
                        runtimeSnapshot.workerPendingWorldActionTicks,
                    (unsigned long long)
                        runtimeSnapshot.workerPendingSaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.workerPendingStopCommands,
                    (unsigned long long)
                        runtimeSnapshot.workerTickCount,
                    (unsigned long long)
                        runtimeSnapshot.completedWorkerActions,
                    (unsigned long long)
                        runtimeSnapshot.processedSaveCommands,
                    (unsigned long long)
                        runtimeSnapshot.processedStopCommands,
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
                        "worker-pending=%llu worker-ticks=%llu "
                        "worker-completions=%llu completed=%s "
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
                            runtimeSnapshot.previousWorkerTickCount,
                        (unsigned long long)
                            runtimeSnapshot.previousCompletedWorkerActions,
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
            if (IsNativeDedicatedServerHostedGameSessionRunning())
            {
                EnqueueNativeDedicatedServerHostedGameWorkerSaveCommand();
            }
            else
            {
                RequestDedicatedServerWorldAutosave(0);
            }
            LogInfo("console", "manual save requested");
            AppendResponseLine(response, "ok manual save requested");
            return true;
        }

        if (command == "stop")
        {
            if (IsNativeDedicatedServerHostedGameSessionRunning())
            {
                EnqueueNativeDedicatedServerHostedGameWorkerStopCommand();
            }
            else
            {
                RequestDedicatedServerShutdown();
            }
            LogInfo("console", "stop requested");
            AppendResponseLine(response, "ok stop requested");
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
