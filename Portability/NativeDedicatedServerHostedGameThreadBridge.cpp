#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameCore.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"

namespace ServerRuntime
{
    bool SignalNativeDedicatedServerHostedGameHostReady();
    void SignalNativeDedicatedServerHostedGameSessionThreadReady(
        std::uint64_t nowMs = 0);
    void SignalNativeDedicatedServerHostedGameSessionThreadStopped(
        std::uint64_t hostedThreadTicks,
        std::uint64_t nowMs = 0);

    namespace
    {
        int RunNativeDedicatedServerHostedGameThread(void *threadParam);

        void SignalNativeDedicatedServerHostedGameThreadReady(
            std::uint64_t nowMs)
        {
            SignalNativeDedicatedServerHostedGameSessionThreadReady(nowMs);
            (void)SignalNativeDedicatedServerHostedGameHostReady();
        }

        void SignalNativeDedicatedServerHostedGameThreadStopped(
            std::uint64_t hostedThreadTicks,
            std::uint64_t nowMs)
        {
            SignalNativeDedicatedServerHostedGameSessionThreadStopped(
                hostedThreadTicks,
                nowMs);
        }

        int RunNativeDedicatedServerHostedGameThread(void *threadParam)
        {
            NativeDedicatedServerHostedGameCoreHooks hooks = {};
            hooks.onThreadReady =
                &SignalNativeDedicatedServerHostedGameThreadReady;
            hooks.onThreadStopped =
                &SignalNativeDedicatedServerHostedGameThreadStopped;
            const NativeDedicatedServerHostedGameCoreRunResult runResult =
                RunNativeDedicatedServerHostedGameCoreWithResult(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hooks);
            if (runResult.finalState.startupResult != 0)
            {
                return runResult.finalState.startupResult;
            }

            return 0;
        }
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return &RunNativeDedicatedServerHostedGameThread;
    }
}
