#pragma once

#include <cstdint>
#include <string>

namespace ServerRuntime
{
    struct NativeDedicatedServerSaveStub
    {
        std::string worldName;
        std::string levelId;
        std::string startupMode;
        std::string sessionPhase;
        std::string hostName;
        std::string bindIp;
        std::string payloadName;
        std::int64_t resolvedSeed = 0;
        std::int64_t payloadBytes = 0;
        std::uint64_t payloadChecksum = 0;
        std::uint64_t saveGeneration = 0;
        std::uint64_t stateChecksum = 0;
        std::uint32_t hostSettings = 0;
        bool dedicatedNoLocalHostPlayer = true;
        unsigned int worldSizeChunks = 0;
        unsigned int worldHellScale = 0;
        bool sessionActive = false;
        bool worldActionIdle = true;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool initialSaveRequested = false;
        bool initialSaveCompleted = false;
        bool initialSaveTimedOut = false;
        bool sessionCompleted = false;
        bool requestedAppShutdown = false;
        bool shutdownHaltedGameplay = false;
        int configuredPort = 0;
        int listenerPort = 0;
        bool onlineGame = true;
        bool privateGame = false;
        bool fakeLocalPlayerJoined = true;
        unsigned int publicSlots = 0;
        unsigned int privateSlots = 0;
        bool startupPayloadPresent = false;
        bool startupPayloadValidated = false;
        std::uint64_t startupThreadIterations = 0;
        std::uint64_t startupThreadDurationMs = 0;
        bool hostedThreadActive = false;
        std::uint64_t hostedThreadTicks = 0;
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
        std::uint64_t autosaveRequests = 0;
        std::uint64_t autosaveCompletions = 0;
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
        unsigned int activeCommandKind = 0;
        std::uint64_t lastProcessedCommandId = 0;
        unsigned int lastProcessedCommandKind = 0;
        std::uint64_t platformTickCount = 0;
        std::uint64_t uptimeMs = 0;
        std::uint64_t gameplayLoopIterations = 0;
        std::uint64_t savedAtFileTime = 0;
    };

    bool BuildNativeDedicatedServerSaveStubText(
        const NativeDedicatedServerSaveStub &stub,
        std::string *outText);

    bool ParseNativeDedicatedServerSaveStubText(
        const std::string &text,
        NativeDedicatedServerSaveStub *outStub);
}
