#include "DedicatedServerWorldBootstrap.h"

#include "StringUtils.h"

namespace ServerRuntime
{
    DedicatedServerWorldBootstrapPlan BuildDedicatedServerWorldBootstrapPlan(
        const ServerPropertiesConfig &serverProperties,
        const WorldBootstrapResult &worldBootstrap)
    {
        DedicatedServerWorldBootstrapPlan plan = {};
        plan.targetWorldName = serverProperties.worldName.empty()
            ? L"world"
            : serverProperties.worldName;
        plan.loadFailed =
            worldBootstrap.status == eWorldBootstrap_Failed;
        plan.createdNewWorld =
            worldBootstrap.status == eWorldBootstrap_CreatedNew;
        plan.resolvedSaveId = worldBootstrap.resolvedSaveId;

        const std::string resolvedLower =
            StringUtils::ToLowerAscii(worldBootstrap.resolvedSaveId);
        const std::string configuredLower =
            StringUtils::ToLowerAscii(serverProperties.worldSaveId);
        plan.shouldPersistResolvedSaveId =
            worldBootstrap.status == eWorldBootstrap_Loaded &&
            !worldBootstrap.resolvedSaveId.empty() &&
            resolvedLower != configuredLower;
        return plan;
    }

    bool ShouldRunDedicatedServerInitialSave(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        bool shutdownRequested,
        bool appShutdown)
    {
        return worldBootstrapPlan.createdNewWorld &&
            !shutdownRequested &&
            !appShutdown;
    }

    DedicatedServerNetworkInitPlan BuildDedicatedServerNetworkInitPlan(
        const DedicatedServerSessionConfig &sessionConfig,
        LoadSaveDataThreadParam *saveData,
        std::int64_t resolvedSeed)
    {
        DedicatedServerNetworkInitPlan plan = {};
        plan.seed = resolvedSeed;
        plan.saveData = saveData;
        plan.settings = sessionConfig.hostSettings;
        plan.dedicatedNoLocalHostPlayer = true;
        plan.worldSizeChunks = sessionConfig.worldSizeChunks;
        plan.worldHellScale = sessionConfig.worldHellScale;
        plan.networkMaxPlayers = sessionConfig.networkMaxPlayers;
        return plan;
    }
}
