#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"

namespace ServerRuntime
{
    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        ResetDedicatedServerHostedGameRuntimeSnapshot();
        RecordDedicatedServerHostedGameRuntimePlan(hostedGamePlan);
        const bool threadInvoked = threadProc != nullptr;
        const int startupResult = threadInvoked
            ? threadProc(threadParam)
            : 0;
        RecordDedicatedServerHostedGameRuntimeStartupResult(
            startupResult,
            threadInvoked);
        return startupResult;
    }
}
