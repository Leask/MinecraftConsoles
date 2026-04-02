#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
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

        void SignalNativeDedicatedServerHostedGameThreadReady()
        {
            SignalNativeDedicatedServerHostedGameSessionThreadReady(
                LceGetMonotonicMilliseconds());
            (void)SignalNativeDedicatedServerHostedGameHostReady();
        }

        void FinalizeNativeDedicatedServerHostedGameThreadStop(
            const NativeDedicatedServerHostedGameCoreRunResult &runResult)
        {
            SignalNativeDedicatedServerHostedGameSessionThreadStopped(
                runResult.finalState.sessionSnapshot.hostedThreadTicks,
                LceGetMonotonicMilliseconds());
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
