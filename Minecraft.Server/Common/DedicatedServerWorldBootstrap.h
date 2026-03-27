#pragma once

#include <cstdint>
#include <string>

#include "../WorldManager.h"
#include "DedicatedServerSessionConfig.h"

namespace ServerRuntime
{
    struct DedicatedServerWorldBootstrapPlan
    {
        std::wstring targetWorldName;
        bool loadFailed = false;
        bool createdNewWorld = false;
        bool shouldPersistResolvedSaveId = false;
        std::string resolvedSaveId;
    };

    struct DedicatedServerNetworkInitPlan
    {
        std::int64_t seed = 0;
        LoadSaveDataThreadParam *saveData = nullptr;
        DWORD settings = 0;
        bool dedicatedNoLocalHostPlayer = true;
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
        unsigned char networkMaxPlayers = 0;
    };

    struct DedicatedServerHostedGamePlan
    {
        std::int64_t resolvedSeed = 0;
        DedicatedServerNetworkInitPlan networkInitPlan = {};
        int localUsersMask = 0;
        bool onlineGame = true;
        bool privateGame = false;
        unsigned char publicSlots = 0;
        unsigned char privateSlots = 0;
        bool fakeLocalPlayerJoined = true;
    };

    DedicatedServerWorldBootstrapPlan BuildDedicatedServerWorldBootstrapPlan(
        const ServerPropertiesConfig &serverProperties,
        const WorldBootstrapResult &worldBootstrap);

    bool ShouldRunDedicatedServerInitialSave(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        bool shutdownRequested,
        bool appShutdown);

    DedicatedServerNetworkInitPlan BuildDedicatedServerNetworkInitPlan(
        const DedicatedServerSessionConfig &sessionConfig,
        LoadSaveDataThreadParam *saveData,
        std::int64_t resolvedSeed);

    std::int64_t ResolveDedicatedServerSeed(
        const DedicatedServerSessionConfig &sessionConfig,
        std::int64_t generatedSeed);

    DedicatedServerHostedGamePlan BuildDedicatedServerHostedGamePlan(
        const DedicatedServerSessionConfig &sessionConfig,
        LoadSaveDataThreadParam *saveData,
        std::int64_t generatedSeed);
}
