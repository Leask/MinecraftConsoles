#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameSession.h"

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameSessionSnapshot
    RunNativeDedicatedServerHostedGameCore(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData);

    namespace
    {
        int RunNativeDedicatedServerHostedGameThread(void *threadParam);

        int RunNativeDedicatedServerHostedGameThread(void *threadParam)
        {
            const NativeDedicatedServerHostedGameSessionSnapshot finalState =
                RunNativeDedicatedServerHostedGameCore(
                    static_cast<
                        NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                            threadParam));
            if (finalState.startup.result != 0)
            {
                return finalState.startup.result;
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
