#include "DedicatedServerHostedGameRuntimeState.h"

#include "NativeDedicatedServerLoadedSaveState.h"
#include "StringUtils.h"

namespace
{
    static constexpr std::uint64_t kDedicatedServerStateChecksumOffset =
        14695981039346656037ULL;
    static constexpr std::uint64_t kDedicatedServerStateChecksumPrime =
        1099511628211ULL;

    ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        g_dedicatedServerHostedGameRuntimeSnapshot = {};

    std::uint64_t ComputeDedicatedServerPayloadChecksum(
        const void *data,
        std::int64_t fileSize)
    {
        if (data == nullptr || fileSize <= 0)
        {
            return 0;
        }

        const unsigned char *bytes =
            static_cast<const unsigned char *>(data);
        std::uint64_t checksum = 14695981039346656037ULL;
        for (std::int64_t i = 0; i < fileSize; ++i)
        {
            checksum ^= bytes[i];
            checksum *= 1099511628211ULL;
        }

        return checksum;
    }

    std::uint64_t MixDedicatedServerStateChecksum(
        std::uint64_t checksum,
        std::uint64_t value)
    {
        for (int i = 0; i < 8; ++i)
        {
            checksum ^= (value & 0xffU);
            checksum *= kDedicatedServerStateChecksumPrime;
            value >>= 8;
        }

        return checksum;
    }

    std::uint64_t HashDedicatedServerStateString(
        const std::string &text,
        std::uint64_t checksum)
    {
        for (size_t i = 0; i < text.size(); ++i)
        {
            checksum ^= (unsigned char)text[i];
            checksum *= kDedicatedServerStateChecksumPrime;
        }

        return checksum;
    }

    void RefreshDedicatedServerHostedGameRuntimeStateChecksum(
        ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot *snapshot)
    {
        if (snapshot == nullptr)
        {
            return;
        }

        std::uint64_t checksum =
            snapshot->previousSessionStateChecksum != 0
                ? snapshot->previousSessionStateChecksum
                : kDedicatedServerStateChecksumOffset;
        checksum = HashDedicatedServerStateString(
            snapshot->worldName,
            checksum);
        checksum = HashDedicatedServerStateString(
            snapshot->worldSaveId,
            checksum);
        checksum = HashDedicatedServerStateString(
            snapshot->savePath,
            checksum);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            (std::uint64_t)snapshot->resolvedSeed);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->savePayloadChecksum);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->saveGeneration);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->acceptedConnections);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->remoteCommands);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->autosaveRequests);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->autosaveCompletions);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->workerPendingWorldActionTicks);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->workerPendingSaveCommands);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->workerPendingStopCommands);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->workerTickCount);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->completedWorkerActions);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->processedSaveCommands);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->processedStopCommands);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->lastQueuedCommandId);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->lastProcessedCommandId);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->lastProcessedCommandKind);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->platformTickCount);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->gameplayLoopIterations);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->hostedThreadTicks);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->sessionActive ? 1U : 0U);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->worldActionIdle ? 1U : 0U);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->appShutdownRequested ? 1U : 0U);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->gameplayHalted ? 1U : 0U);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->stopSignalValid ? 1U : 0U);
        checksum = MixDedicatedServerStateChecksum(
            checksum,
            snapshot->sessionCompleted ? 1U : 0U);
        snapshot->sessionStateChecksum = checksum;
    }

    void UpdateDedicatedServerHostedGameRuntimeUptime(
        ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot *snapshot,
        std::uint64_t nowMs)
    {
        if (snapshot == nullptr)
        {
            return;
        }

        snapshot->lastUpdateMs = nowMs;
        if (snapshot->sessionStartMs == 0 || nowMs < snapshot->sessionStartMs)
        {
            snapshot->uptimeMs = 0;
            return;
        }

        snapshot->uptimeMs = nowMs - snapshot->sessionStartMs;
    }

    ServerRuntime::EDedicatedServerHostedGameRuntimePhase
    ResolveDedicatedServerHostedGameRuntimePhase(
        const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
            &snapshot,
        bool appShutdownRequested,
        bool gameplayHalted)
    {
        if (!snapshot.sessionActive)
        {
            return snapshot.phase;
        }

        if (gameplayHalted || appShutdownRequested)
        {
            return
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_ShutdownRequested;
        }

        return ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Running;
    }
}

namespace ServerRuntime
{
    void ResetDedicatedServerHostedGameRuntimeSnapshot()
    {
        g_dedicatedServerHostedGameRuntimeSnapshot =
            DedicatedServerHostedGameRuntimeSnapshot{};
    }

    void RecordDedicatedServerHostedGameRuntimePlan(
        const DedicatedServerHostedGamePlan &hostedGamePlan)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot =
            DedicatedServerHostedGameRuntimeSnapshot{};
        g_dedicatedServerHostedGameRuntimeSnapshot.phase =
            eDedicatedServerHostedGameRuntimePhase_Startup;
        g_dedicatedServerHostedGameRuntimeSnapshot.startAttempted = true;
        g_dedicatedServerHostedGameRuntimeSnapshot.loadedFromSave =
            hostedGamePlan.networkInitPlan.saveData != nullptr;
        g_dedicatedServerHostedGameRuntimeSnapshot.onlineGame =
            hostedGamePlan.onlineGame;
        g_dedicatedServerHostedGameRuntimeSnapshot.privateGame =
            hostedGamePlan.privateGame;
        g_dedicatedServerHostedGameRuntimeSnapshot.fakeLocalPlayerJoined =
            hostedGamePlan.fakeLocalPlayerJoined;
        g_dedicatedServerHostedGameRuntimeSnapshot.resolvedSeed =
            hostedGamePlan.resolvedSeed;
        g_dedicatedServerHostedGameRuntimeSnapshot.hostSettings =
            hostedGamePlan.networkInitPlan.settings;
        g_dedicatedServerHostedGameRuntimeSnapshot
            .dedicatedNoLocalHostPlayer =
                hostedGamePlan.networkInitPlan.dedicatedNoLocalHostPlayer;
        if (hostedGamePlan.networkInitPlan.saveData != nullptr)
        {
            g_dedicatedServerHostedGameRuntimeSnapshot.savePayloadBytes =
                hostedGamePlan.networkInitPlan.saveData->fileSize;
            g_dedicatedServerHostedGameRuntimeSnapshot.savePayloadChecksum =
                ComputeDedicatedServerPayloadChecksum(
                    hostedGamePlan.networkInitPlan.saveData->data,
                    hostedGamePlan.networkInitPlan.saveData->fileSize);
            g_dedicatedServerHostedGameRuntimeSnapshot.savePayloadName =
                StringUtils::WideToUtf8(
                    hostedGamePlan.networkInitPlan.saveData->saveName);
            const NativeDedicatedServerLoadedSaveMetadata loadedSaveMetadata =
                GetNativeDedicatedServerLoadedSaveMetadata();
            if (loadedSaveMetadata.available)
            {
                g_dedicatedServerHostedGameRuntimeSnapshot.loadedSavePath =
                    loadedSaveMetadata.savePath;
            }
            if (loadedSaveMetadata.hasSaveStub)
            {
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .loadedSaveMetadataAvailable = true;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousStartupMode =
                        loadedSaveMetadata.saveStub.startupMode;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousSessionPhase =
                        loadedSaveMetadata.saveStub.sessionPhase;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousRemoteCommands =
                        loadedSaveMetadata.saveStub.remoteCommands;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousAutosaveCompletions =
                        loadedSaveMetadata.saveStub.autosaveCompletions;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousWorkerPendingWorldActionTicks =
                        loadedSaveMetadata.saveStub
                            .workerPendingWorldActionTicks;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousWorkerPendingSaveCommands =
                        loadedSaveMetadata.saveStub
                            .workerPendingSaveCommands;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousWorkerPendingStopCommands =
                        loadedSaveMetadata.saveStub
                            .workerPendingStopCommands;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousWorkerTickCount =
                        loadedSaveMetadata.saveStub.workerTickCount;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousCompletedWorkerActions =
                        loadedSaveMetadata.saveStub
                            .completedWorkerActions;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousProcessedSaveCommands =
                        loadedSaveMetadata.saveStub
                            .processedSaveCommands;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousProcessedStopCommands =
                        loadedSaveMetadata.saveStub
                            .processedStopCommands;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousLastQueuedCommandId =
                        loadedSaveMetadata.saveStub
                            .lastQueuedCommandId;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousLastProcessedCommandId =
                        loadedSaveMetadata.saveStub
                            .lastProcessedCommandId;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousLastProcessedCommandKind =
                        loadedSaveMetadata.saveStub
                            .lastProcessedCommandKind;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousPlatformTickCount =
                        loadedSaveMetadata.saveStub.platformTickCount;
                g_dedicatedServerHostedGameRuntimeSnapshot.previousUptimeMs =
                    loadedSaveMetadata.saveStub.uptimeMs;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousGameplayLoopIterations =
                        loadedSaveMetadata.saveStub.gameplayLoopIterations;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousHostSettings =
                        loadedSaveMetadata.saveStub.hostSettings;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousDedicatedNoLocalHostPlayer =
                        loadedSaveMetadata.saveStub
                            .dedicatedNoLocalHostPlayer;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousWorldSizeChunks =
                        loadedSaveMetadata.saveStub.worldSizeChunks;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousWorldHellScale =
                        (unsigned char)
                            loadedSaveMetadata.saveStub.worldHellScale;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousOnlineGame =
                        loadedSaveMetadata.saveStub.onlineGame;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousPrivateGame =
                        loadedSaveMetadata.saveStub.privateGame;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousFakeLocalPlayerJoined =
                        loadedSaveMetadata.saveStub.fakeLocalPlayerJoined;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousPublicSlots =
                        (unsigned char)
                            loadedSaveMetadata.saveStub.publicSlots;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousPrivateSlots =
                        (unsigned char)
                            loadedSaveMetadata.saveStub.privateSlots;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousSavePayloadChecksum =
                        loadedSaveMetadata.saveStub.payloadChecksum;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousSaveGeneration =
                        loadedSaveMetadata.saveStub.saveGeneration;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousSessionStateChecksum =
                        loadedSaveMetadata.saveStub.stateChecksum;
                g_dedicatedServerHostedGameRuntimeSnapshot.saveGeneration =
                    loadedSaveMetadata.saveStub.saveGeneration;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .sessionStateChecksum =
                        loadedSaveMetadata.saveStub.stateChecksum;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousStartupPayloadPresent =
                        loadedSaveMetadata.saveStub.startupPayloadPresent;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousStartupPayloadValidated =
                        loadedSaveMetadata.saveStub.startupPayloadValidated;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousStartupThreadIterations =
                        loadedSaveMetadata.saveStub.startupThreadIterations;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousStartupThreadDurationMs =
                        loadedSaveMetadata.saveStub.startupThreadDurationMs;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousHostedThreadActive =
                        loadedSaveMetadata.saveStub.hostedThreadActive;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousHostedThreadTicks =
                        loadedSaveMetadata.saveStub.hostedThreadTicks;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousSessionCompleted =
                        loadedSaveMetadata.saveStub.sessionCompleted;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousRequestedAppShutdown =
                        loadedSaveMetadata.saveStub.requestedAppShutdown;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousShutdownHaltedGameplay =
                        loadedSaveMetadata.saveStub.shutdownHaltedGameplay;
            }
        }
        g_dedicatedServerHostedGameRuntimeSnapshot.worldSizeChunks =
            hostedGamePlan.networkInitPlan.worldSizeChunks;
        g_dedicatedServerHostedGameRuntimeSnapshot.worldHellScale =
            hostedGamePlan.networkInitPlan.worldHellScale;
        g_dedicatedServerHostedGameRuntimeSnapshot.publicSlots =
            hostedGamePlan.publicSlots;
        g_dedicatedServerHostedGameRuntimeSnapshot.privateSlots =
            hostedGamePlan.privateSlots;
    }

    void RecordDedicatedServerHostedGameRuntimeStartupResult(
        int startupResult,
        bool threadInvoked)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.startupResult =
            startupResult;
        g_dedicatedServerHostedGameRuntimeSnapshot.threadInvoked =
            threadInvoked;
        g_dedicatedServerHostedGameRuntimeSnapshot.sessionActive =
            startupResult == 0;
        g_dedicatedServerHostedGameRuntimeSnapshot.phase =
            startupResult == 0
                ? eDedicatedServerHostedGameRuntimePhase_Startup
                : eDedicatedServerHostedGameRuntimePhase_Failed;
    }

    void RecordDedicatedServerHostedGameRuntimeSessionContext(
        const DedicatedServerHostedGameRuntimeSessionContext &sessionContext,
        std::uint64_t sessionStartMs)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.worldName =
            sessionContext.worldName;
        g_dedicatedServerHostedGameRuntimeSnapshot.worldSaveId =
            sessionContext.worldSaveId;
        g_dedicatedServerHostedGameRuntimeSnapshot.savePath =
            sessionContext.savePath;
        g_dedicatedServerHostedGameRuntimeSnapshot.storageRoot =
            sessionContext.storageRoot;
        g_dedicatedServerHostedGameRuntimeSnapshot.hostName =
            sessionContext.hostName;
        g_dedicatedServerHostedGameRuntimeSnapshot.bindIp =
            sessionContext.bindIp;
        g_dedicatedServerHostedGameRuntimeSnapshot.configuredPort =
            sessionContext.configuredPort;
        g_dedicatedServerHostedGameRuntimeSnapshot.listenerPort =
            sessionContext.listenerPort;
        g_dedicatedServerHostedGameRuntimeSnapshot.sessionStartMs =
            sessionStartMs;
        g_dedicatedServerHostedGameRuntimeSnapshot.stoppedMs = 0;
        g_dedicatedServerHostedGameRuntimeSnapshot.phase =
            eDedicatedServerHostedGameRuntimePhase_Running;
        UpdateDedicatedServerHostedGameRuntimeUptime(
            &g_dedicatedServerHostedGameRuntimeSnapshot,
            sessionStartMs);
        RefreshDedicatedServerHostedGameRuntimeStateChecksum(
            &g_dedicatedServerHostedGameRuntimeSnapshot);
    }

    void UpdateDedicatedServerHostedGameRuntimeSessionState(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        std::uint64_t autosaveRequests,
        std::uint64_t autosaveCompletions,
        std::uint64_t platformTickCount,
        bool worldActionIdle,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t nowMs)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.acceptedConnections =
            acceptedConnections;
        g_dedicatedServerHostedGameRuntimeSnapshot.remoteCommands =
            remoteCommands;
        g_dedicatedServerHostedGameRuntimeSnapshot.autosaveRequests =
            autosaveRequests;
        g_dedicatedServerHostedGameRuntimeSnapshot.autosaveCompletions =
            autosaveCompletions;
        g_dedicatedServerHostedGameRuntimeSnapshot.saveGeneration =
            g_dedicatedServerHostedGameRuntimeSnapshot
                .previousSaveGeneration +
            autosaveCompletions;
        g_dedicatedServerHostedGameRuntimeSnapshot.platformTickCount =
            platformTickCount;
        g_dedicatedServerHostedGameRuntimeSnapshot.worldActionIdle =
            worldActionIdle;
        g_dedicatedServerHostedGameRuntimeSnapshot.appShutdownRequested =
            appShutdownRequested;
        g_dedicatedServerHostedGameRuntimeSnapshot.gameplayHalted =
            gameplayHalted;
        g_dedicatedServerHostedGameRuntimeSnapshot.stopSignalValid =
            stopSignalValid;
        g_dedicatedServerHostedGameRuntimeSnapshot.phase =
            ResolveDedicatedServerHostedGameRuntimePhase(
                g_dedicatedServerHostedGameRuntimeSnapshot,
                appShutdownRequested,
                gameplayHalted);
        UpdateDedicatedServerHostedGameRuntimeUptime(
            &g_dedicatedServerHostedGameRuntimeSnapshot,
            nowMs);
        RefreshDedicatedServerHostedGameRuntimeStateChecksum(
            &g_dedicatedServerHostedGameRuntimeSnapshot);
    }

    void MarkDedicatedServerHostedGameRuntimeSessionStopped(
        std::uint64_t stoppedMs)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.sessionActive = false;
        g_dedicatedServerHostedGameRuntimeSnapshot.stoppedMs = stoppedMs;
        g_dedicatedServerHostedGameRuntimeSnapshot.phase =
            eDedicatedServerHostedGameRuntimePhase_Stopped;
        UpdateDedicatedServerHostedGameRuntimeUptime(
            &g_dedicatedServerHostedGameRuntimeSnapshot,
            stoppedMs);
        RefreshDedicatedServerHostedGameRuntimeStateChecksum(
            &g_dedicatedServerHostedGameRuntimeSnapshot);
    }

    void RecordDedicatedServerHostedGameRuntimeGameplayLoopIteration(
        std::uint64_t gameplayLoopIterations)
    {
        if (gameplayLoopIterations >=
            g_dedicatedServerHostedGameRuntimeSnapshot
                .gameplayLoopIterations)
        {
            g_dedicatedServerHostedGameRuntimeSnapshot
                .gameplayLoopIterations = gameplayLoopIterations;
            RefreshDedicatedServerHostedGameRuntimeStateChecksum(
                &g_dedicatedServerHostedGameRuntimeSnapshot);
        }
    }

    void RecordDedicatedServerHostedGameRuntimeStartupTelemetry(
        bool startupPayloadPresent,
        bool startupPayloadValidated,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.startupPayloadPresent =
            startupPayloadPresent;
        g_dedicatedServerHostedGameRuntimeSnapshot.startupPayloadValidated =
            startupPayloadValidated;
        g_dedicatedServerHostedGameRuntimeSnapshot.startupThreadIterations =
            startupThreadIterations;
        g_dedicatedServerHostedGameRuntimeSnapshot.startupThreadDurationMs =
            startupThreadDurationMs;
    }

    void RecordDedicatedServerHostedGameRuntimeThreadState(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.hostedThreadActive =
            hostedThreadActive;
        g_dedicatedServerHostedGameRuntimeSnapshot.hostedThreadTicks =
            hostedThreadTicks;
        RefreshDedicatedServerHostedGameRuntimeStateChecksum(
            &g_dedicatedServerHostedGameRuntimeSnapshot);
    }

    void RecordDedicatedServerHostedGameRuntimeCoreState(
        std::uint64_t saveGeneration,
        std::uint64_t sessionStateChecksum)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.saveGeneration =
            saveGeneration;
        g_dedicatedServerHostedGameRuntimeSnapshot.sessionStateChecksum =
            sessionStateChecksum;
    }

    void RecordDedicatedServerHostedGameRuntimeWorkerState(
        std::uint64_t workerPendingWorldActionTicks,
        std::uint64_t workerPendingSaveCommands,
        std::uint64_t workerPendingStopCommands,
        std::uint64_t workerTickCount,
        std::uint64_t completedWorkerActions,
        std::uint64_t processedSaveCommands,
        std::uint64_t processedStopCommands,
        std::uint64_t lastQueuedCommandId,
        std::uint64_t lastProcessedCommandId,
        unsigned int lastProcessedCommandKind)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot
            .workerPendingWorldActionTicks =
                workerPendingWorldActionTicks;
        g_dedicatedServerHostedGameRuntimeSnapshot
            .workerPendingSaveCommands =
                workerPendingSaveCommands;
        g_dedicatedServerHostedGameRuntimeSnapshot
            .workerPendingStopCommands =
                workerPendingStopCommands;
        g_dedicatedServerHostedGameRuntimeSnapshot.workerTickCount =
            g_dedicatedServerHostedGameRuntimeSnapshot
                .previousWorkerTickCount +
            workerTickCount;
        g_dedicatedServerHostedGameRuntimeSnapshot.completedWorkerActions =
            g_dedicatedServerHostedGameRuntimeSnapshot
                .previousCompletedWorkerActions +
            completedWorkerActions;
        g_dedicatedServerHostedGameRuntimeSnapshot.processedSaveCommands =
            g_dedicatedServerHostedGameRuntimeSnapshot
                .previousProcessedSaveCommands +
            processedSaveCommands;
        g_dedicatedServerHostedGameRuntimeSnapshot.processedStopCommands =
            g_dedicatedServerHostedGameRuntimeSnapshot
                .previousProcessedStopCommands +
            processedStopCommands;
        g_dedicatedServerHostedGameRuntimeSnapshot.lastQueuedCommandId =
            lastQueuedCommandId;
        g_dedicatedServerHostedGameRuntimeSnapshot.lastProcessedCommandId =
            lastProcessedCommandId;
        g_dedicatedServerHostedGameRuntimeSnapshot.lastProcessedCommandKind =
            lastProcessedCommandKind;
        RefreshDedicatedServerHostedGameRuntimeStateChecksum(
            &g_dedicatedServerHostedGameRuntimeSnapshot);
    }

    void RecordDedicatedServerHostedGameRuntimeSessionSummary(
        const DedicatedServerHostedGameRuntimeSessionSummary &summary)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.initialSaveRequested =
            summary.initialSaveRequested;
        g_dedicatedServerHostedGameRuntimeSnapshot.initialSaveCompleted =
            summary.initialSaveCompleted;
        g_dedicatedServerHostedGameRuntimeSnapshot.initialSaveTimedOut =
            summary.initialSaveTimedOut;
        g_dedicatedServerHostedGameRuntimeSnapshot.sessionCompleted =
            summary.sessionCompleted;
        g_dedicatedServerHostedGameRuntimeSnapshot.requestedAppShutdown =
            summary.requestedAppShutdown;
        g_dedicatedServerHostedGameRuntimeSnapshot.shutdownHaltedGameplay =
            summary.shutdownHaltedGameplay;
        if (summary.gameplayLoopIterations >=
            g_dedicatedServerHostedGameRuntimeSnapshot
                .gameplayLoopIterations)
        {
            g_dedicatedServerHostedGameRuntimeSnapshot
                .gameplayLoopIterations =
                    summary.gameplayLoopIterations;
        }
        RefreshDedicatedServerHostedGameRuntimeStateChecksum(
            &g_dedicatedServerHostedGameRuntimeSnapshot);
    }

    void RecordDedicatedServerHostedGameRuntimePersistedSave(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.lastPersistedSavePath =
            savePath;
        g_dedicatedServerHostedGameRuntimeSnapshot.lastPersistedFileTime =
            savedAtFileTime;
        g_dedicatedServerHostedGameRuntimeSnapshot
            .lastPersistedAutosaveCompletions = autosaveCompletions;
    }

    const char *GetDedicatedServerHostedGameRuntimePhaseName(
        EDedicatedServerHostedGameRuntimePhase phase)
    {
        switch (phase)
        {
        case eDedicatedServerHostedGameRuntimePhase_Startup:
            return "startup";
        case eDedicatedServerHostedGameRuntimePhase_Running:
            return "running";
        case eDedicatedServerHostedGameRuntimePhase_ShutdownRequested:
            return "shutdown-requested";
        case eDedicatedServerHostedGameRuntimePhase_Stopped:
            return "stopped";
        case eDedicatedServerHostedGameRuntimePhase_Failed:
            return "failed";
        case eDedicatedServerHostedGameRuntimePhase_Idle:
        default:
            return "idle";
        }
    }

    DedicatedServerHostedGameRuntimeSnapshot
    GetDedicatedServerHostedGameRuntimeSnapshot()
    {
        return g_dedicatedServerHostedGameRuntimeSnapshot;
    }
}
