#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameStartup.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThreadBridge.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        int CompletePersistentNativeDedicatedServerHostedGameRuntimeStartup(
            const NativeDedicatedServerHostedGameHostStartResult &startResult)
        {
            const std::uint64_t nowMs = LceGetMonotonicMilliseconds();
            if (startResult.sessionSnapshotAvailable)
            {
                ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                    startResult.sessionSnapshot,
                    startResult.startupResult,
                    startResult.threadInvoked,
                    nowMs);
            }
            else
            {
                ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                    startResult.startupResult,
                    startResult.threadInvoked,
                    nowMs);
            }
            return startResult.startupResult;
        }
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        const bool persistentSession =
            threadProc == GetNativeDedicatedServerHostedGameThreadProc();
        if (!persistentSession &&
            !BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        const NativeDedicatedServerHostedGameHostStartResult result =
            StartNativeDedicatedServerHostedGameRuntimePath(
                hostedGamePlan,
                threadProc,
                threadParam);

        if (persistentSession)
        {
            return CompletePersistentNativeDedicatedServerHostedGameRuntimeStartup(
                result);
        }

        return CompleteDedicatedServerHostedGameRuntimeStartup(
            result.startupResult,
            result.threadInvoked);
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return GetNativeDedicatedServerHostedGameThreadProc();
    }
}
