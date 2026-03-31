#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameStartup.h"
#include "NativeDedicatedServerHostedGameThreadBridge.h"

namespace ServerRuntime
{
    void ActivateDedicatedServerHostedGamePlan(
        const DedicatedServerHostedGamePlan &)
    {
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        return StartNativeDedicatedServerHostedGameRuntimeAndComplete(
            hostedGamePlan,
            threadProc,
            threadParam);
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return GetNativeDedicatedServerHostedGameThreadProc();
    }
}
