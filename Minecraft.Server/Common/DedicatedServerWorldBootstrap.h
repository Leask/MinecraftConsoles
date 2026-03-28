#pragma once

#include <cstdint>
#include <string>

#include <lce_win32/lce_win32.h>

#include "DedicatedServerWorldTypes.h"
#include "DedicatedServerSessionConfig.h"

namespace ServerRuntime
{
    enum EDedicatedServerWorldLoadStatus
    {
        eDedicatedServerWorldLoad_Loaded,
        eDedicatedServerWorldLoad_NotFound,
        eDedicatedServerWorldLoad_Failed
    };

    struct DedicatedServerWorldTarget
    {
        std::wstring worldName;
        std::string saveId;
    };

    struct DedicatedServerWorldBootstrapPlan
    {
        std::wstring targetWorldName;
        bool loadFailed = false;
        bool createdNewWorld = false;
        bool shouldPersistResolvedSaveId = false;
        std::string resolvedSaveId;
    };

    typedef void (*DedicatedServerSetWorldTitleProc)(
        const std::wstring &worldName,
        void *context);
    typedef void (*DedicatedServerSetWorldSaveIdProc)(
        const std::string &saveId,
        void *context);
    typedef void (*DedicatedServerResetWorldSaveDataProc)(void *context);

    struct DedicatedServerWorldStorageHooks
    {
        DedicatedServerSetWorldTitleProc setWorldTitleProc = nullptr;
        void *setWorldTitleContext = nullptr;
        DedicatedServerSetWorldSaveIdProc setWorldSaveIdProc = nullptr;
        void *setWorldSaveIdContext = nullptr;
        DedicatedServerResetWorldSaveDataProc resetSaveDataProc = nullptr;
        void *resetSaveDataContext = nullptr;
    };

    struct DedicatedServerWorldStoragePlan
    {
        std::wstring worldName;
        std::string saveId;
        bool shouldResetSaveData = false;
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

    struct DedicatedServerWorldLoadPlan
    {
        bool shouldPersistResolvedSaveId = false;
        std::string resolvedSaveId;
        bool shouldAbortStartup = false;
        int abortExitCode = 0;
    };

    struct DedicatedServerInitialSavePlan
    {
        bool shouldRequestInitialSave = false;
        DWORD idleWaitBeforeRequestMs = 5000;
        DWORD requestTimeoutMs = 30000;
    };

    struct DedicatedServerHostedThreadStartupPlan
    {
        bool shouldAbortStartup = false;
        int abortExitCode = 0;
    };

    DedicatedServerWorldTarget ResolveDedicatedServerWorldTarget(
        const ServerPropertiesConfig &serverProperties);

    DedicatedServerWorldStoragePlan
    BuildConfiguredDedicatedServerWorldStoragePlan(
        const DedicatedServerWorldTarget &worldTarget);

    DedicatedServerWorldStoragePlan
    BuildLoadedDedicatedServerWorldStoragePlan(
        const DedicatedServerWorldTarget &worldTarget,
        const std::string &resolvedLoadedSaveId);

    DedicatedServerWorldStoragePlan
    BuildCreatedDedicatedServerWorldStoragePlan(
        const DedicatedServerWorldTarget &worldTarget);

    bool ApplyDedicatedServerWorldStoragePlan(
        const DedicatedServerWorldStoragePlan &plan,
        const DedicatedServerWorldStorageHooks &hooks);

    WorldBootstrapResult BuildDedicatedServerWorldBootstrapResult(
        EDedicatedServerWorldLoadStatus loadStatus,
        LoadSaveDataThreadParam *saveData,
        const DedicatedServerWorldTarget &worldTarget,
        const std::string &resolvedLoadedSaveId);

    DedicatedServerWorldBootstrapPlan BuildDedicatedServerWorldBootstrapPlan(
        const ServerPropertiesConfig &serverProperties,
        const WorldBootstrapResult &worldBootstrap);

    DedicatedServerWorldLoadPlan BuildDedicatedServerWorldLoadPlan(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan);

    DedicatedServerInitialSavePlan BuildDedicatedServerInitialSavePlan(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        bool shutdownRequested,
        bool appShutdown);

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

    DedicatedServerHostedThreadStartupPlan
    BuildDedicatedServerHostedThreadStartupPlan(int startupResult);

    template <typename TInitData>
    void PopulateDedicatedServerNetworkGameInitData(
        TInitData *initData,
        const DedicatedServerNetworkInitPlan &networkInitPlan)
    {
        if (initData == nullptr)
        {
            return;
        }

        initData->seed = networkInitPlan.seed;
#ifdef _LARGE_WORLDS
        initData->xzSize = networkInitPlan.worldSizeChunks;
        initData->hellScale = networkInitPlan.worldHellScale;
#endif
        initData->saveData = networkInitPlan.saveData;
        initData->settings = networkInitPlan.settings;
        initData->dedicatedNoLocalHostPlayer =
            networkInitPlan.dedicatedNoLocalHostPlayer;
    }
}
