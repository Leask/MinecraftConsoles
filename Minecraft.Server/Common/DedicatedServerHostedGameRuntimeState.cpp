#include "DedicatedServerHostedGameRuntimeState.h"

#include "NativeDedicatedServerLoadedSaveState.h"
#include "StringUtils.h"

namespace
{
    ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        g_dedicatedServerHostedGameRuntimeSnapshot = {};

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
        if (hostedGamePlan.networkInitPlan.saveData != nullptr)
        {
            g_dedicatedServerHostedGameRuntimeSnapshot.savePayloadBytes =
                hostedGamePlan.networkInitPlan.saveData->fileSize;
            g_dedicatedServerHostedGameRuntimeSnapshot.savePayloadName =
                StringUtils::WideToUtf8(
                    hostedGamePlan.networkInitPlan.saveData->saveName);
            const NativeDedicatedServerLoadedSaveMetadata loadedSaveMetadata =
                GetNativeDedicatedServerLoadedSaveMetadata();
            if (loadedSaveMetadata.available)
            {
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .loadedSaveMetadataAvailable = true;
                g_dedicatedServerHostedGameRuntimeSnapshot.loadedSavePath =
                    loadedSaveMetadata.savePath;
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
                    .previousPlatformTickCount =
                        loadedSaveMetadata.saveStub.platformTickCount;
                g_dedicatedServerHostedGameRuntimeSnapshot.previousUptimeMs =
                    loadedSaveMetadata.saveStub.uptimeMs;
                g_dedicatedServerHostedGameRuntimeSnapshot
                    .previousGameplayLoopIterations =
                        loadedSaveMetadata.saveStub.gameplayLoopIterations;
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
    }

    void RecordDedicatedServerHostedGameRuntimeGameplayLoopIteration(
        std::uint64_t gameplayLoopIterations)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.gameplayLoopIterations =
            gameplayLoopIterations;
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
        g_dedicatedServerHostedGameRuntimeSnapshot.gameplayLoopIterations =
            summary.gameplayLoopIterations;
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
