#include "DedicatedServerHostedGameRuntime.h"

#include "DedicatedServerHostedGameRuntimeState.h"

namespace ServerRuntime
{
    bool BeginDedicatedServerHostedGameRuntimeStartup(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        int *outFailureResult)
    {
        ResetDedicatedServerHostedGameRuntimeSnapshot();
        RecordDedicatedServerHostedGameRuntimePlan(hostedGamePlan);
        if (threadProc != nullptr)
        {
            return true;
        }

        if (outFailureResult != nullptr)
        {
            *outFailureResult = -1;
        }
        RecordDedicatedServerHostedGameRuntimeStartupResult(-1, false);
        return false;
    }

    int CompleteDedicatedServerHostedGameRuntimeStartup(
        int startupResult,
        bool threadInvoked)
    {
        RecordDedicatedServerHostedGameRuntimeStartupResult(
            startupResult,
            threadInvoked);
        return startupResult;
    }
}
