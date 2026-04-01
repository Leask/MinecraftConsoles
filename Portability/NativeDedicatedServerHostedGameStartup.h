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

    bool PrepareNativeDedicatedServerHostedGameStartup(
        bool persistentSession,
        HANDLE *outStartupReadyEvent);

    int CompleteNativeDedicatedServerHostedGameStartup(
        bool persistentSession,
        const NativeDedicatedServerHostedGameRuntimeStartResult &startResult,
        std::uint64_t nowMs = 0);

    void PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const DedicatedServerHostedGamePlan &hostedGamePlan);

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
