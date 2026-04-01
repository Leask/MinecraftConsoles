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
    namespace
    {
        void ActivateDedicatedServerHostedGamePlan(
            const DedicatedServerHostedGamePlan &hostedGamePlan)
        {
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
        }
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return &CGameNetworkManager::RunNetworkGameThreadProc;
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        if (!BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        ActivateDedicatedServerHostedGamePlan(hostedGamePlan);

        C4JThread startThread(threadProc, threadParam, "RunNetworkGame");
        startThread.Run();

        while (startThread.isRunning() &&
            !IsDedicatedServerShutdownRequested())
        {
            TickDedicatedServerPlatformRuntime();
            LceSleepMilliseconds(10);
        }

        startThread.WaitForCompletion(INFINITE);
        startupResult = startThread.GetExitCode();
        return CompleteDedicatedServerHostedGameRuntimeStartup(
            startupResult,
            true);
    }
}
