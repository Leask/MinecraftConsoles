#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
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
        int RunNativeDedicatedServerHostedGameThread(void *threadParam);

        DedicatedServerHostedGameThreadProc
            *GetNativeDedicatedServerHostedGameThreadProc()
        {
            return &RunNativeDedicatedServerHostedGameThread;
        }

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
                runResult.finalState.sessionSnapshot.hostedThreadTicks,
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

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return GetNativeDedicatedServerHostedGameThreadProc();
    }
}
