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
            ObserveNativeDedicatedServerHostedGameSessionThreadStateAndWorkerProject(
                running,
                GetNativeDedicatedServerHostedGameSessionThreadTicks(),
                LceGetMonotonicMilliseconds());
        }

        void SignalNativeDedicatedServerHostedGameThreadReady()
        {
            SyncNativeDedicatedServerHostedThreadState(true);
            (void)SignalNativeDedicatedServerHostedGameHostReady();
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
            hooks.onThreadStopped =
                &FinalizeNativeDedicatedServerHostedGameThreadStop;
            return RunNativeDedicatedServerHostedGameCore(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hooks);
        }
    }

    void ProjectNativeDedicatedServerHostedGameThreadSnapshot()
    {
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot();
    }

    void FinalizeNativeDedicatedServerHostedGameThreadStop()
    {
        SyncNativeDedicatedServerHostedThreadState(false);
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
