#pragma once

#include "DedicatedServerWorldBootstrap.h"

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

    void ActivateDedicatedServerHostedGamePlan(
        const DedicatedServerHostedGamePlan &hostedGamePlan);

    bool BeginDedicatedServerHostedGameRuntimeStartup(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        int *outFailureResult);

    int CompleteDedicatedServerHostedGameRuntimeStartup(
        int startupResult,
        bool threadInvoked);

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
