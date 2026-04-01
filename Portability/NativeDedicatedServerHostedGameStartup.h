#pragma once

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThread.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameRuntimeStartResult
    {
        bool startupReady = false;
        bool threadInvoked = false;
        bool sessionSnapshotAvailable = false;
        int startupResult = -1;
        NativeDedicatedServerHostedGameSessionSnapshot sessionSnapshot = {};
    };

    NativeDedicatedServerHostedGameRuntimeStartResult
    StartNativeDedicatedServerHostedGameRuntimePath(
        bool persistentSession,
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks);

    int StartNativeDedicatedServerHostedGameRuntimeAndComplete(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam);
}
