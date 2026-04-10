#pragma once

#include <cstdint>
#include <string>

namespace ServerRuntime
{
    enum ENativeDedicatedServerHostedGameWorkerCommandKind
    {
        eNativeDedicatedServerHostedGameWorkerCommand_None = 0,
        eNativeDedicatedServerHostedGameWorkerCommand_Autosave = 1,
        eNativeDedicatedServerHostedGameWorkerCommand_Save = 2,
        eNativeDedicatedServerHostedGameWorkerCommand_Stop = 3,
        eNativeDedicatedServerHostedGameWorkerCommand_Halt = 4
    };

    struct NativeDedicatedServerHostedGameWorkerSnapshot
    {
        std::uint64_t pendingWorldActionTicks = 0;
        std::uint64_t pendingAutosaveCommands = 0;
        std::uint64_t pendingSaveCommands = 0;
        std::uint64_t pendingStopCommands = 0;
        std::uint64_t pendingHaltCommands = 0;
        std::uint64_t workerTickCount = 0;
        std::uint64_t completedWorldActions = 0;
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
    };

    struct NativeDedicatedServerHostedGameStartupSnapshot
    {
        int result = 0;
        bool payloadValidated = false;
        std::uint64_t threadIterations = 0;
        std::uint64_t threadDurationMs = 0;
    };

    struct NativeDedicatedServerHostedGamePreviousStartupSnapshot
    {
        std::string mode;
        bool payloadPresent = false;
        bool payloadValidated = false;
        std::uint64_t threadIterations = 0;
        std::uint64_t threadDurationMs = 0;
    };

    struct NativeDedicatedServerHostedGameSessionSummarySnapshot
    {
        bool initialSaveRequested = false;
        bool initialSaveCompleted = false;
        bool initialSaveTimedOut = false;
        bool sessionCompleted = false;
        bool requestedAppShutdown = false;
        bool shutdownHaltedGameplay = false;
    };

    struct NativeDedicatedServerHostedGameActivationSnapshot
    {
        bool onlineGame = false;
        bool privateGame = false;
        bool fakeLocalPlayerJoined = false;
        unsigned int publicSlots = 0;
        unsigned int privateSlots = 0;
    };

    struct NativeDedicatedServerHostedGameWorldConfigSnapshot
    {
        std::uint32_t hostSettings = 0;
        bool dedicatedNoLocalHostPlayer = true;
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
    };

    struct NativeDedicatedServerHostedGamePayloadSnapshot
    {
        std::string name;
        std::int64_t bytes = 0;
        std::uint64_t checksum = 0;
    };

    struct NativeDedicatedServerHostedGamePreviousPayloadSnapshot
    {
        std::uint64_t checksum = 0;
    };

    struct NativeDedicatedServerHostedGamePersistedSaveSnapshot
    {
        std::string savePath;
        std::uint64_t fileTime = 0;
        std::uint64_t autosaveCompletions = 0;
    };

    struct NativeDedicatedServerHostedGameSessionContextSnapshot
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

    struct NativeDedicatedServerHostedGameLoadedSaveSnapshot
    {
        bool metadataAvailable = false;
        std::string path;
    };

    struct NativeDedicatedServerHostedGameSessionProgressSnapshot
    {
        std::uint64_t sessionTicks = 0;
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
        std::uint64_t autosaveRequests = 0;
        std::uint64_t observedAutosaveCompletions = 0;
        std::uint64_t gameplayLoopIterations = 0;
        std::uint64_t platformTickCount = 0;
        std::uint64_t saveGeneration = 0;
        std::uint64_t stateChecksum = 0;
    };

    struct NativeDedicatedServerHostedGamePreviousProgressSnapshot
    {
        std::uint64_t remoteCommands = 0;
        std::uint64_t autosaveCompletions = 0;
        std::uint64_t platformTickCount = 0;
        std::uint64_t uptimeMs = 0;
        std::uint64_t gameplayLoopIterations = 0;
        std::uint64_t saveGeneration = 0;
        std::uint64_t sessionStateChecksum = 0;
    };

    struct NativeDedicatedServerHostedGameSessionSnapshot
    {
        bool startAttempted = false;
        bool active = false;
        bool loadedFromSave = false;
        bool threadInvoked = false;
        std::string previousSessionPhase;
        int runtimePhase = 0;
        int localUsersMask = 0;
        std::int64_t resolvedSeed = 0;
        NativeDedicatedServerHostedGameWorkerSnapshot previousWorker = {};
        NativeDedicatedServerHostedGameWorkerSnapshot worker = {};
        std::uint64_t sessionStartMs = 0;
        NativeDedicatedServerHostedGamePreviousStartupSnapshot
            previousStartup = {};
        NativeDedicatedServerHostedGameStartupSnapshot startup = {};
        NativeDedicatedServerHostedGameSessionSummarySnapshot
            previousSummary = {};
        NativeDedicatedServerHostedGameSessionSummarySnapshot summary = {};
        NativeDedicatedServerHostedGameActivationSnapshot
            previousActivation = {};
        NativeDedicatedServerHostedGameActivationSnapshot activation = {};
        NativeDedicatedServerHostedGameWorldConfigSnapshot
            previousWorldConfig = {};
        NativeDedicatedServerHostedGameWorldConfigSnapshot worldConfig = {};
        NativeDedicatedServerHostedGamePreviousPayloadSnapshot
            previousPayload = {};
        NativeDedicatedServerHostedGamePayloadSnapshot payload = {};
        NativeDedicatedServerHostedGamePersistedSaveSnapshot
            persistedSave = {};
        NativeDedicatedServerHostedGameSessionContextSnapshot context = {};
        NativeDedicatedServerHostedGameLoadedSaveSnapshot loadedSave = {};
        NativeDedicatedServerHostedGameSessionProgressSnapshot progress = {};
        NativeDedicatedServerHostedGamePreviousProgressSnapshot
            previousProgress = {};
        std::uint64_t previousHostedThreadTicks = 0;
        std::uint64_t hostedThreadTicks = 0;
        std::uint64_t stoppedMs = 0;
        bool worldActionIdle = true;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool stopSignalValid = false;
        bool hostedThreadActive = false;
        bool previousHostedThreadActive = false;
    };

}
