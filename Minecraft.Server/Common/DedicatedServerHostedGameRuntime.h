#pragma once

#include "DedicatedServerWorldBootstrap.h"
#include "NativeDedicatedServerHostedGameRuntimeStub.h"

namespace ServerRuntime
{
    typedef int DedicatedServerHostedGameThreadProc(void* threadParam);

    struct DedicatedServerHostedGameStartupExecutionResult
    {
        int startupResult = 0;
        DedicatedServerHostedThreadStartupPlan startupPlan = {};
    };

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam);

    bool BeginDedicatedServerHostedGameRuntimeStartup(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        int *outFailureResult);

    int CompleteDedicatedServerHostedGameRuntimeStartup(
        int startupResult,
        bool threadInvoked);

    inline void PopulateDedicatedServerHostedGameRuntimeStubInitData(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const DedicatedServerHostedGamePlan &hostedGamePlan)
    {
        if (initData == nullptr)
        {
            return;
        }

        initData->localUsersMask = hostedGamePlan.localUsersMask;
        initData->onlineGame = hostedGamePlan.onlineGame;
        initData->privateGame = hostedGamePlan.privateGame;
        initData->publicSlots = hostedGamePlan.publicSlots;
        initData->privateSlots = hostedGamePlan.privateSlots;
        initData->fakeLocalPlayerJoined =
            hostedGamePlan.fakeLocalPlayerJoined;
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc();

    template <typename TInitData>
    DedicatedServerHostedGameStartupExecutionResult
    ExecuteDedicatedServerHostedGameStartup(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        TInitData *initData)
    {
        DedicatedServerHostedGameStartupExecutionResult result = {};
        PopulateDedicatedServerNetworkGameInitData(
            initData,
            hostedGamePlan.networkInitPlan);
        result.startupResult = StartDedicatedServerHostedGameRuntime(
            hostedGamePlan,
            threadProc,
            initData);
        result.startupPlan =
            BuildDedicatedServerHostedThreadStartupPlan(
                result.startupResult);
        return result;
    }
}
