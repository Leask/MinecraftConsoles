#include "stdafx.h"

#include "..\Common\DedicatedServerHostedGameRuntime.h"
#include "..\Common\DedicatedServerHostedGameRuntimeState.h"
#include "..\Common\DedicatedServerPlatformRuntime.h"
#include "..\Common\DedicatedServerSignalState.h"

#include "Common/Network/GameNetworkManager.h"
#include "..\..\Minecraft.World\C4JThread.h"

#include <lce_time/lce_time.h>

namespace ServerRuntime
{
    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        ResetDedicatedServerHostedGameRuntimeSnapshot();
        RecordDedicatedServerHostedGameRuntimePlan(hostedGamePlan);
        if (threadProc == nullptr)
        {
            RecordDedicatedServerHostedGameRuntimeStartupResult(-1, false);
            return -1;
        }

        g_NetworkManager.HostGame(
            hostedGamePlan.localUsersMask,
            hostedGamePlan.onlineGame,
            hostedGamePlan.privateGame,
            hostedGamePlan.publicSlots,
            hostedGamePlan.privateSlots);
        if (hostedGamePlan.fakeLocalPlayerJoined)
        {
            g_NetworkManager.FakeLocalPlayerJoined();
        }

        C4JThread startThread(threadProc, threadParam, "RunNetworkGame");
        startThread.Run();

        while (startThread.isRunning() &&
            !IsDedicatedServerShutdownRequested())
        {
            TickDedicatedServerPlatformRuntime();
            LceSleepMilliseconds(10);
        }

        startThread.WaitForCompletion(INFINITE);
        const int startupResult = startThread.GetExitCode();
        RecordDedicatedServerHostedGameRuntimeStartupResult(
            startupResult,
            true);
        return startupResult;
    }
}
