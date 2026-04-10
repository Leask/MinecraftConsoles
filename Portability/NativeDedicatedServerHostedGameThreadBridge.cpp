#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameSessionSnapshot
    RunNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        void (*onThreadReady)(std::uint64_t nowMs),
        void (*onThreadStopped)(
            std::uint64_t hostedThreadTicks,
            std::uint64_t nowMs));

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
            const NativeDedicatedServerHostedGameSessionSnapshot finalState =
                RunNativeDedicatedServerHostedGameCoreWithResult(
                    static_cast<
                        NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                            threadParam),
                    &SignalNativeDedicatedServerHostedGameThreadReady,
                    &SignalNativeDedicatedServerHostedGameThreadStopped);
            if (finalState.startupResult != 0)
            {
                return finalState.startupResult;
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
