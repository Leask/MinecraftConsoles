#pragma once

#include <cstdint>
#include <string>

#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    struct DedicatedServerHostedGameRuntimeSnapshot
    {
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
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
        unsigned char publicSlots = 0;
        unsigned char privateSlots = 0;
        std::string worldName;
        std::string worldSaveId;
        std::string storageRoot;
        std::string hostName;
        std::string bindIp;
        std::string savePayloadName;
        int configuredPort = 0;
        int listenerPort = 0;
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
        std::uint64_t autosaveRequests = 0;
        std::uint64_t autosaveCompletions = 0;
        std::uint64_t sessionStartMs = 0;
        std::uint64_t lastUpdateMs = 0;
        std::uint64_t stoppedMs = 0;
        std::uint64_t uptimeMs = 0;
    };

    struct DedicatedServerHostedGameRuntimeSessionContext
    {
        std::string worldName;
        std::string worldSaveId;
        std::string storageRoot;
        std::string hostName;
        std::string bindIp;
        int configuredPort = 0;
        int listenerPort = 0;
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
        std::uint64_t nowMs);

    void MarkDedicatedServerHostedGameRuntimeSessionStopped(
        std::uint64_t stoppedMs);

    DedicatedServerHostedGameRuntimeSnapshot
    GetDedicatedServerHostedGameRuntimeSnapshot();
}
