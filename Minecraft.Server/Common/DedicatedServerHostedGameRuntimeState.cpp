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
    }

    void RecordDedicatedServerHostedGameRuntimeSessionContext(
        const DedicatedServerHostedGameRuntimeSessionContext &sessionContext,
        std::uint64_t sessionStartMs)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.worldName =
            sessionContext.worldName;
        g_dedicatedServerHostedGameRuntimeSnapshot.worldSaveId =
            sessionContext.worldSaveId;
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
        UpdateDedicatedServerHostedGameRuntimeUptime(
            &g_dedicatedServerHostedGameRuntimeSnapshot,
            nowMs);
    }

    void MarkDedicatedServerHostedGameRuntimeSessionStopped(
        std::uint64_t stoppedMs)
    {
        g_dedicatedServerHostedGameRuntimeSnapshot.sessionActive = false;
        g_dedicatedServerHostedGameRuntimeSnapshot.stoppedMs = stoppedMs;
        UpdateDedicatedServerHostedGameRuntimeUptime(
            &g_dedicatedServerHostedGameRuntimeSnapshot,
            stoppedMs);
    }

    DedicatedServerHostedGameRuntimeSnapshot
    GetDedicatedServerHostedGameRuntimeSnapshot()
    {
        return g_dedicatedServerHostedGameRuntimeSnapshot;
    }
}
