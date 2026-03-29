#pragma once

#include <cstdint>
#include <string>

#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    enum EDedicatedServerHostedGameRuntimePhase
    {
        eDedicatedServerHostedGameRuntimePhase_Idle,
        eDedicatedServerHostedGameRuntimePhase_Startup,
        eDedicatedServerHostedGameRuntimePhase_Running,
        eDedicatedServerHostedGameRuntimePhase_ShutdownRequested,
        eDedicatedServerHostedGameRuntimePhase_Stopped,
        eDedicatedServerHostedGameRuntimePhase_Failed
    };

    struct DedicatedServerHostedGameRuntimeSnapshot
    {
        EDedicatedServerHostedGameRuntimePhase phase =
            eDedicatedServerHostedGameRuntimePhase_Idle;
        bool startAttempted = false;
        bool threadInvoked = false;
        bool sessionActive = false;
        bool loadedFromSave = false;
        bool onlineGame = false;
        bool privateGame = false;
        bool fakeLocalPlayerJoined = false;
        int startupResult = 0;
        std::int64_t resolvedSeed = 0;
        std::int64_t savePayloadBytes = 0;
        std::uint64_t savePayloadChecksum = 0;
        std::uint32_t hostSettings = 0;
        bool dedicatedNoLocalHostPlayer = true;
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
        unsigned char publicSlots = 0;
        unsigned char privateSlots = 0;
        std::string worldName;
        std::string worldSaveId;
        std::string savePath;
        std::string storageRoot;
        std::string hostName;
        std::string bindIp;
        std::string savePayloadName;
        std::string previousSessionPhase;
        bool loadedSaveMetadataAvailable = false;
        std::string loadedSavePath;
        std::string previousStartupMode;
        int configuredPort = 0;
        int listenerPort = 0;
        std::uint32_t previousHostSettings = 0;
        bool previousDedicatedNoLocalHostPlayer = true;
        unsigned int previousWorldSizeChunks = 0;
        unsigned char previousWorldHellScale = 0;
        bool worldActionIdle = true;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool stopSignalValid = false;
        std::uint64_t previousSavePayloadChecksum = 0;
        bool previousStartupPayloadPresent = false;
        bool previousStartupPayloadValidated = false;
        std::uint64_t previousStartupThreadIterations = 0;
        std::uint64_t previousStartupThreadDurationMs = 0;
        std::uint64_t previousRemoteCommands = 0;
        std::uint64_t previousAutosaveCompletions = 0;
        std::uint64_t previousPlatformTickCount = 0;
        std::uint64_t previousUptimeMs = 0;
        std::uint64_t previousGameplayLoopIterations = 0;
        bool previousSessionCompleted = false;
        bool previousRequestedAppShutdown = false;
        bool previousShutdownHaltedGameplay = false;
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
        std::uint64_t autosaveRequests = 0;
        std::uint64_t autosaveCompletions = 0;
        std::uint64_t platformTickCount = 0;
        bool initialSaveRequested = false;
        bool initialSaveCompleted = false;
        bool initialSaveTimedOut = false;
        bool sessionCompleted = false;
        bool requestedAppShutdown = false;
        bool shutdownHaltedGameplay = false;
        std::uint64_t gameplayLoopIterations = 0;
        bool startupPayloadPresent = false;
        bool startupPayloadValidated = false;
        std::uint64_t startupThreadIterations = 0;
        std::uint64_t startupThreadDurationMs = 0;
        std::string lastPersistedSavePath;
        std::uint64_t lastPersistedFileTime = 0;
        std::uint64_t lastPersistedAutosaveCompletions = 0;
        std::uint64_t sessionStartMs = 0;
        std::uint64_t lastUpdateMs = 0;
        std::uint64_t stoppedMs = 0;
        std::uint64_t uptimeMs = 0;
    };

    struct DedicatedServerHostedGameRuntimeSessionContext
    {
        std::string worldName;
        std::string worldSaveId;
        std::string savePath;
        std::string storageRoot;
        std::string hostName;
        std::string bindIp;
        int configuredPort = 0;
        int listenerPort = 0;
    };

    struct DedicatedServerHostedGameRuntimeSessionSummary
    {
        bool initialSaveRequested = false;
        bool initialSaveCompleted = false;
        bool initialSaveTimedOut = false;
        bool sessionCompleted = false;
        bool requestedAppShutdown = false;
        bool shutdownHaltedGameplay = false;
        std::uint64_t gameplayLoopIterations = 0;
    };

    void ResetDedicatedServerHostedGameRuntimeSnapshot();

    void RecordDedicatedServerHostedGameRuntimePlan(
        const DedicatedServerHostedGamePlan &hostedGamePlan);

    void RecordDedicatedServerHostedGameRuntimeStartupResult(
        int startupResult,
        bool threadInvoked);

    void RecordDedicatedServerHostedGameRuntimeSessionContext(
        const DedicatedServerHostedGameRuntimeSessionContext &sessionContext,
        std::uint64_t sessionStartMs);

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
        std::uint64_t nowMs);

    void RecordDedicatedServerHostedGameRuntimeGameplayLoopIteration(
        std::uint64_t gameplayLoopIterations);

    void RecordDedicatedServerHostedGameRuntimeStartupTelemetry(
        bool startupPayloadPresent,
        bool startupPayloadValidated,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs);

    void RecordDedicatedServerHostedGameRuntimeSessionSummary(
        const DedicatedServerHostedGameRuntimeSessionSummary &summary);

    void MarkDedicatedServerHostedGameRuntimeSessionStopped(
        std::uint64_t stoppedMs);

    void RecordDedicatedServerHostedGameRuntimePersistedSave(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions);

    const char *GetDedicatedServerHostedGameRuntimePhaseName(
        EDedicatedServerHostedGameRuntimePhase phase);

    DedicatedServerHostedGameRuntimeSnapshot
    GetDedicatedServerHostedGameRuntimeSnapshot();
}
