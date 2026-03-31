#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameStartup.h"
#include "NativeDedicatedServerHostedGameThreadBridge.h"
#include "lce_time/lce_time.h"

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
        int startupResult = 0;
        const bool usePersistentNativeSession =
            threadProc == GetNativeDedicatedServerHostedGameThreadProc();
        if (!usePersistentNativeSession &&
            !BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        ActivateDedicatedServerHostedGamePlan(hostedGamePlan);

        const NativeDedicatedServerHostedGameThreadCallbacks callbacks =
            BuildNativeDedicatedServerHostedGameThreadCallbacks();

        const NativeDedicatedServerHostedGameRuntimeStartResult result =
            StartNativeDedicatedServerHostedGameRuntimePath(
                usePersistentNativeSession,
                hostedGamePlan,
                threadProc,
                threadParam,
                callbacks);
        if (usePersistentNativeSession && result.startupReady)
        {
            ProjectNativeDedicatedServerHostedGameThreadSnapshot();
        }

        return CompleteNativeDedicatedServerHostedGameStartup(
            usePersistentNativeSession,
            result.startupResult,
            result.threadInvoked,
            LceGetMonotonicMilliseconds());
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return GetNativeDedicatedServerHostedGameThreadProc();
    }
}
