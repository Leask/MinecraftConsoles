#include "NativeDedicatedServerHostedGameThreadBridge.h"

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameCore.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        void SyncNativeDedicatedServerHostedThreadState(bool running)
        {
            if (running)
            {
                SignalNativeDedicatedServerHostedGameSessionThreadReady(
                    LceGetMonotonicMilliseconds());
                return;
            }

            SignalNativeDedicatedServerHostedGameSessionThreadStopped(
                GetNativeDedicatedServerHostedGameSessionThreadTicks(),
                LceGetMonotonicMilliseconds());
        }

        void SignalNativeDedicatedServerHostedGameThreadReady()
        {
            SyncNativeDedicatedServerHostedThreadState(true);
            (void)SignalNativeDedicatedServerHostedGameHostReady();
        }

        void FinalizeNativeDedicatedServerHostedGameThreadStop(
            const NativeDedicatedServerHostedGameCoreRunResult &runResult)
        {
            SignalNativeDedicatedServerHostedGameSessionThreadStopped(
                runResult.finalSessionSnapshot.hostedThreadTicks,
                LceGetMonotonicMilliseconds());
        }

        void TickNativeDedicatedServerHostedGameThreadPlatformRuntime()
        {
            TickDedicatedServerPlatformRuntime();
        }

        void HandleNativeDedicatedServerHostedGameThreadPlatformActions()
        {
            HandleDedicatedServerPlatformActions();
        }

        int RunNativeDedicatedServerHostedGameThread(void *threadParam)
        {
            NativeDedicatedServerHostedGameCoreHooks hooks = {};
            hooks.onThreadReady =
                &SignalNativeDedicatedServerHostedGameThreadReady;
            const NativeDedicatedServerHostedGameCoreRunResult runResult =
                RunNativeDedicatedServerHostedGameCoreWithResult(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hooks);
            FinalizeNativeDedicatedServerHostedGameThreadStop(runResult);
            return runResult.exitCode;
        }
    }

    void ProjectNativeDedicatedServerHostedGameThreadSnapshot()
    {
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot();
    }
    NativeDedicatedServerHostedGameThreadCallbacks
    BuildNativeDedicatedServerHostedGameThreadCallbacks()
    {
        NativeDedicatedServerHostedGameThreadCallbacks callbacks = {};
        callbacks.tickPlatformRuntime =
            &TickNativeDedicatedServerHostedGameThreadPlatformRuntime;
        callbacks.handlePlatformActions =
            &HandleNativeDedicatedServerHostedGameThreadPlatformActions;
        return callbacks;
    }

    DedicatedServerHostedGameThreadProc
        *GetNativeDedicatedServerHostedGameThreadProc()
    {
        return &RunNativeDedicatedServerHostedGameThread;
    }
}
