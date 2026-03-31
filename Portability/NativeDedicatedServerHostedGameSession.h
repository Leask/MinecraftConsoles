#pragma once

#include <cstdint>
#include <string>

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameWorker.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameSessionSnapshot
    {
        bool startAttempted = false;
        bool active = false;
        bool loadedFromSave = false;
        bool payloadValidated = false;
        bool threadInvoked = false;
        bool onlineGame = false;
        bool privateGame = false;
        bool fakeLocalPlayerJoined = false;
        bool dedicatedNoLocalHostPlayer = true;
        bool loadedSaveMetadataAvailable = false;
        std::string worldName;
        std::string worldSaveId;
        std::string savePath;
        std::string storageRoot;
        std::string hostName;
        std::string bindIp;
        std::string savePayloadName;
        std::string loadedSavePath;
        std::string previousStartupMode;
        std::string previousSessionPhase;
        int startupResult = 0;
        int runtimePhase = 0;
        int localUsersMask = 0;
        int configuredPort = 0;
        int listenerPort = 0;
        std::int64_t resolvedSeed = 0;
        std::int64_t savePayloadBytes = 0;
        std::uint32_t hostSettings = 0;
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
        unsigned int publicSlots = 0;
        unsigned int privateSlots = 0;
        std::uint64_t payloadChecksum = 0;
        std::uint64_t sessionTicks = 0;
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
        std::uint64_t autosaveRequests = 0;
        std::uint64_t observedAutosaveCompletions = 0;
        std::uint64_t previousRemoteCommands = 0;
        std::uint64_t previousAutosaveCompletions = 0;
        std::uint64_t previousWorkerPendingWorldActionTicks = 0;
        std::uint64_t previousWorkerPendingAutosaveCommands = 0;
        std::uint64_t previousWorkerPendingSaveCommands = 0;
        std::uint64_t previousWorkerPendingStopCommands = 0;
        std::uint64_t previousWorkerPendingHaltCommands = 0;
        std::uint64_t previousWorkerTickCount = 0;
        std::uint64_t previousCompletedWorkerActions = 0;
        std::uint64_t previousProcessedAutosaveCommands = 0;
        std::uint64_t previousProcessedSaveCommands = 0;
        std::uint64_t previousProcessedStopCommands = 0;
        std::uint64_t previousProcessedHaltCommands = 0;
        std::uint64_t previousLastQueuedCommandId = 0;
        std::uint64_t previousActiveCommandId = 0;
        std::uint64_t previousActiveCommandTicksRemaining = 0;
        std::uint64_t previousLastProcessedCommandId = 0;
        std::uint64_t workerPendingWorldActionTicks = 0;
        std::uint64_t workerPendingAutosaveCommands = 0;
        std::uint64_t workerPendingSaveCommands = 0;
        std::uint64_t workerPendingStopCommands = 0;
        std::uint64_t workerPendingHaltCommands = 0;
        std::uint64_t workerTickCount = 0;
        std::uint64_t completedWorkerActions = 0;
        std::uint64_t processedAutosaveCommands = 0;
        std::uint64_t processedSaveCommands = 0;
        std::uint64_t processedStopCommands = 0;
        std::uint64_t processedHaltCommands = 0;
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
        std::uint64_t previousPlatformTickCount = 0;
        std::uint64_t previousUptimeMs = 0;
        std::uint64_t previousGameplayLoopIterations = 0;
        std::uint64_t previousSavePayloadChecksum = 0;
        std::uint64_t previousSaveGeneration = 0;
        std::uint64_t previousSessionStateChecksum = 0;
        std::uint64_t sessionStartMs = 0;
        std::uint64_t lastPersistedFileTime = 0;
        std::uint64_t lastPersistedAutosaveCompletions = 0;
        std::string lastPersistedSavePath;
        std::uint64_t previousStartupThreadIterations = 0;
        std::uint64_t previousStartupThreadDurationMs = 0;
        std::uint64_t startupThreadIterations = 0;
        std::uint64_t startupThreadDurationMs = 0;
        std::uint64_t previousHostedThreadTicks = 0;
        std::uint64_t hostedThreadTicks = 0;
        std::uint64_t stoppedMs = 0;
        std::uint32_t previousHostSettings = 0;
        unsigned int previousWorldSizeChunks = 0;
        unsigned char previousWorldHellScale = 0;
        unsigned char previousPublicSlots = 0;
        unsigned char previousPrivateSlots = 0;
        bool worldActionIdle = true;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool stopSignalValid = false;
        bool hostedThreadActive = false;
        bool previousOnlineGame = false;
        bool previousPrivateGame = false;
        bool previousFakeLocalPlayerJoined = false;
        bool previousDedicatedNoLocalHostPlayer = true;
        bool previousStartupPayloadPresent = false;
        bool previousStartupPayloadValidated = false;
        bool previousHostedThreadActive = false;
        bool initialSaveRequested = false;
        bool initialSaveCompleted = false;
        bool initialSaveTimedOut = false;
        bool sessionCompleted = false;
        bool requestedAppShutdown = false;
        bool shutdownHaltedGameplay = false;
        bool previousSessionCompleted = false;
        bool previousRequestedAppShutdown = false;
        bool previousShutdownHaltedGameplay = false;
        ENativeDedicatedServerHostedGameWorkerCommandKind
            previousActiveCommandKind =
                eNativeDedicatedServerHostedGameWorkerCommand_None;
        ENativeDedicatedServerHostedGameWorkerCommandKind
            previousLastProcessedCommandKind =
                eNativeDedicatedServerHostedGameWorkerCommand_None;
    };

    struct NativeDedicatedServerHostedGameSessionPersistContext
    {
        std::string worldName;
        std::string worldSaveId;
        std::string hostName;
        std::string bindIp;
        int configuredPort = 0;
        int listenerPort = 0;
    };

    void ResetNativeDedicatedServerHostedGameSessionState();

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData,
        bool startupPayloadValidated);

    void ObserveNativeDedicatedServerHostedGameSessionStartupResult(
        int startupResult,
        bool threadInvoked);

    void ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs = 0);

    void TickNativeDedicatedServerHostedGameSession(
        bool hostedThreadActive = true);

    void TickNativeDedicatedServerHostedGameSessionFrame(
        const NativeDedicatedServerHostedGameWorkerSnapshot &workerSnapshot,
        std::uint64_t autosaveCompletions,
        bool hostedThreadActive = true);

    void ObserveNativeDedicatedServerHostedGameSessionAutosaves(
        std::uint64_t autosaveCompletions);

    void ObserveNativeDedicatedServerHostedGameSessionAutosavesAndProject(
        std::uint64_t autosaveCompletions,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionActivity(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle);

    void ObserveNativeDedicatedServerHostedGameSessionActivityAndProject(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionContext(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &savePath,
        const std::string &storageRoot,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
        std::uint64_t sessionStartMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionContextAndProject(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &savePath,
        const std::string &storageRoot,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
        std::uint64_t sessionStartMs = 0,
        std::uint64_t nowMs = 0);

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

    void ObserveNativeDedicatedServerHostedGameSessionPlatformStateAndProject(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid);

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeStateAndProject(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions);

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSaveAndProject(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionSummary(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay);

    void FinalizeNativeDedicatedServerHostedGameSession(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay,
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t stoppedMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionThreadState(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks);

    void ObserveNativeDedicatedServerHostedGameSessionThreadStateAndProject(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionStartupTelemetry(
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs);

    void ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionWorkerState(
        std::uint64_t pendingWorldActionTicks,
        std::uint64_t pendingAutosaveCommands,
        std::uint64_t pendingSaveCommands,
        std::uint64_t pendingStopCommands,
        std::uint64_t pendingHaltCommands,
        std::uint64_t workerTickCount,
        std::uint64_t completedWorkerActions,
        std::uint64_t processedAutosaveCommands,
        std::uint64_t processedSaveCommands,
        std::uint64_t processedStopCommands,
        std::uint64_t processedHaltCommands,
        std::uint64_t lastQueuedCommandId,
        std::uint64_t activeCommandId,
        std::uint64_t activeCommandTicksRemaining,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            activeCommandKind,
        std::uint64_t lastProcessedCommandId,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            lastProcessedCommandKind);

    void ObserveNativeDedicatedServerHostedGameSessionWorkerSnapshot(
        const NativeDedicatedServerHostedGameWorkerSnapshot &workerSnapshot);

    void ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
        std::uint64_t nowMs = 0);

    void RequestNativeDedicatedServerHostedGameSessionAutosave(
        unsigned int workTicks,
        std::uint64_t nowMs = 0);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionSaveCommand(
        std::uint64_t nowMs = 0);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionStopCommand(
        std::uint64_t nowMs = 0);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionHaltSequence(
        bool requestAutosaveFirst,
        unsigned int autosaveWorkTicks,
        std::uint64_t nowMs = 0);

    void ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
        std::uint64_t nowMs = 0);

    void StopNativeDedicatedServerHostedGameSession(
        std::uint64_t stoppedMs = 0);

    bool IsNativeDedicatedServerHostedGameSessionRunning();

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    bool BuildNativeDedicatedServerSaveStubFromSessionSnapshot(
        const NativeDedicatedServerHostedGameSessionPersistContext
            &persistContext,
        std::uint64_t nowMs,
        std::uint64_t savedAtFileTime,
        NativeDedicatedServerSaveStub *outSaveStub);

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);
}
