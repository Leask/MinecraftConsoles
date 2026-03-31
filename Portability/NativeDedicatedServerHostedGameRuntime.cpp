#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameStartup.h"
#include "NativeDedicatedServerHostedGameThreadBridge.h"
#include "NativeDedicatedServerHostedGameWorker.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include "lce_win32/lce_win32.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameSessionCoreState();

    void ActivateDedicatedServerHostedGamePlan(
        const DedicatedServerHostedGamePlan &)
    {
    }

    void ResetNativeDedicatedServerHostedGameSessionState()
    {
        ResetNativeDedicatedServerHostedGameHostState();
        ResetNativeDedicatedServerHostedGameWorkerState();
        ResetNativeDedicatedServerHostedGameSessionCoreState();
        ProjectNativeDedicatedServerHostedGameThreadSnapshot();
    }

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode)
    {
        if (outExitCode != nullptr)
        {
            *outExitCode = 0;
        }

        const bool stopped = WaitForNativeDedicatedServerHostedGameHostStop(
            timeoutMs,
            outExitCode);
        if (stopped)
        {
            FinalizeNativeDedicatedServerHostedGameThreadStop();
        }
        return stopped;
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
