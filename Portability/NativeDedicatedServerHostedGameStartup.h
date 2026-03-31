#pragma once

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameThread.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGamePersistentStartResult
    {
        bool ready = false;
        bool threadInvoked = false;
        int startupResult = -1;
    };

    bool PrepareNativeDedicatedServerHostedGameStartup(
        bool persistentSession,
        HANDLE *outStartupReadyEvent);

    int CompleteNativeDedicatedServerHostedGameStartup(
        bool persistentSession,
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs = 0);

    void PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const DedicatedServerHostedGamePlan &hostedGamePlan);

    NativeDedicatedServerHostedGamePersistentStartResult
    StartPersistentNativeDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks);
}
