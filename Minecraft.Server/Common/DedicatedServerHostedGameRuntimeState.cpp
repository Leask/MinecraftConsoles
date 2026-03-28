#include "DedicatedServerHostedGameRuntimeState.h"

namespace
{
    ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        g_dedicatedServerHostedGameRuntimeSnapshot = {};
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
    }

    DedicatedServerHostedGameRuntimeSnapshot
    GetDedicatedServerHostedGameRuntimeSnapshot()
    {
        return g_dedicatedServerHostedGameRuntimeSnapshot;
    }
}
