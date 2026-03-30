#pragma once

#include <cstdint>
#include <string>

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameWorker.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameSessionSnapshot
    {
        bool active = false;
        bool loadedFromSave = false;
        bool payloadValidated = false;
        bool threadInvoked = false;
        bool onlineGame = false;
        bool privateGame = false;
        bool fakeLocalPlayerJoined = false;
        std::string worldName;
        std::string worldSaveId;
        std::string savePath;
        std::string storageRoot;
        std::string hostName;
        std::string bindIp;
        int startupResult = 0;
        int runtimePhase = 0;
        int localUsersMask = 0;
        int configuredPort = 0;
        int listenerPort = 0;
        unsigned int publicSlots = 0;
        unsigned int privateSlots = 0;
        std::uint64_t payloadChecksum = 0;
        std::uint64_t sessionTicks = 0;
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
        std::uint64_t autosaveRequests = 0;
        std::uint64_t observedAutosaveCompletions = 0;
        std::uint64_t workerPendingWorldActionTicks = 0;
        std::uint64_t workerPendingAutosaveCommands = 0;
        std::uint64_t workerPendingSaveCommands = 0;
        std::uint64_t workerPendingStopCommands = 0;
        std::uint64_t workerTickCount = 0;
        std::uint64_t completedWorkerActions = 0;
        std::uint64_t processedAutosaveCommands = 0;
        std::uint64_t processedSaveCommands = 0;
        std::uint64_t processedStopCommands = 0;
        std::uint64_t lastQueuedCommandId = 0;
        std::uint64_t activeCommandId = 0;
        std::uint64_t activeCommandTicksRemaining = 0;
        ENativeDedicatedServerHostedGameWorkerCommandKind
            activeCommandKind =
                eNativeDedicatedServerHostedGameWorkerCommand_None;
        std::uint64_t lastProcessedCommandId = 0;
        ENativeDedicatedServerHostedGameWorkerCommandKind
            lastProcessedCommandKind =
                eNativeDedicatedServerHostedGameWorkerCommand_None;
        std::uint64_t gameplayLoopIterations = 0;
        std::uint64_t platformTickCount = 0;
        std::uint64_t saveGeneration = 0;
        std::uint64_t stateChecksum = 0;
        std::uint64_t lastPersistedFileTime = 0;
        std::uint64_t lastPersistedAutosaveCompletions = 0;
        std::uint64_t startupThreadIterations = 0;
        std::uint64_t startupThreadDurationMs = 0;
        std::uint64_t hostedThreadTicks = 0;
        bool worldActionIdle = true;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool stopSignalValid = false;
        bool hostedThreadActive = false;
        bool initialSaveRequested = false;
        bool initialSaveCompleted = false;
        bool initialSaveTimedOut = false;
        bool sessionCompleted = false;
        bool requestedAppShutdown = false;
        bool shutdownHaltedGameplay = false;
    };

    void ResetNativeDedicatedServerHostedGameSessionState();

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData,
        bool startupPayloadValidated);

    void ObserveNativeDedicatedServerHostedGameSessionStartupResult(
        int startupResult,
        bool threadInvoked);

    void TickNativeDedicatedServerHostedGameSession();

    void ObserveNativeDedicatedServerHostedGameSessionAutosaves(
        std::uint64_t autosaveCompletions);

    void ObserveNativeDedicatedServerHostedGameSessionActivity(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle);

    void ObserveNativeDedicatedServerHostedGameSessionContext(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &savePath,
        const std::string &storageRoot,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort);

    void ObserveNativeDedicatedServerHostedGameSessionActivation(
        int localUsersMask,
        bool onlineGame,
        bool privateGame,
        unsigned int publicSlots,
        unsigned int privateSlots,
        bool fakeLocalPlayerJoined);

    void ObserveNativeDedicatedServerHostedGameSessionPlatformState(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount);

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid);

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions);

    void ObserveNativeDedicatedServerHostedGameSessionSummary(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay);

    void ObserveNativeDedicatedServerHostedGameSessionPhase(
        int runtimePhase);

    void ObserveNativeDedicatedServerHostedGameSessionThreadState(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks);

    void ObserveNativeDedicatedServerHostedGameSessionStartupTelemetry(
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs);

    void ObserveNativeDedicatedServerHostedGameSessionWorkerState(
        std::uint64_t pendingWorldActionTicks,
        std::uint64_t pendingAutosaveCommands,
        std::uint64_t pendingSaveCommands,
        std::uint64_t pendingStopCommands,
        std::uint64_t workerTickCount,
        std::uint64_t completedWorkerActions,
        std::uint64_t processedAutosaveCommands,
        std::uint64_t processedSaveCommands,
        std::uint64_t processedStopCommands,
        std::uint64_t lastQueuedCommandId,
        std::uint64_t activeCommandId,
        std::uint64_t activeCommandTicksRemaining,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            activeCommandKind,
        std::uint64_t lastProcessedCommandId,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            lastProcessedCommandKind);

    void StopNativeDedicatedServerHostedGameSession();

    bool IsNativeDedicatedServerHostedGameSessionRunning();

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);
}
