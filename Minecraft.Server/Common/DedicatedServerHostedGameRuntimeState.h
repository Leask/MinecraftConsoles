#pragma once

#include <cstdint>

#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    struct DedicatedServerHostedGameRuntimeSnapshot
    {
        bool startAttempted = false;
        bool threadInvoked = false;
        bool loadedFromSave = false;
        bool onlineGame = false;
        bool privateGame = false;
        bool fakeLocalPlayerJoined = false;
        int startupResult = 0;
        std::int64_t resolvedSeed = 0;
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
        unsigned char publicSlots = 0;
        unsigned char privateSlots = 0;
    };

    void ResetDedicatedServerHostedGameRuntimeSnapshot();

    void RecordDedicatedServerHostedGameRuntimePlan(
        const DedicatedServerHostedGamePlan &hostedGamePlan);

    void RecordDedicatedServerHostedGameRuntimeStartupResult(
        int startupResult,
        bool threadInvoked);

    DedicatedServerHostedGameRuntimeSnapshot
    GetDedicatedServerHostedGameRuntimeSnapshot();
}
