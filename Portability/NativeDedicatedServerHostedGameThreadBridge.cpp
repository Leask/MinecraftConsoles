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

        void SignalNativeDedicatedServerHostedGameThreadReady()
        {
            SignalNativeDedicatedServerHostedGameSessionThreadReady(
                LceGetMonotonicMilliseconds());
            (void)SignalNativeDedicatedServerHostedGameHostReady();
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
            SignalNativeDedicatedServerHostedGameSessionThreadStopped(
                runResult.finalState.sessionSnapshot.hostedThreadTicks,
                LceGetMonotonicMilliseconds());
            return runResult.exitCode;
        }
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return &RunNativeDedicatedServerHostedGameThread;
    }
}
