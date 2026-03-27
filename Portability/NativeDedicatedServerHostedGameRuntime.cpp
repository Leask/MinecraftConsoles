#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"

namespace ServerRuntime
{
    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        return threadProc != nullptr
            ? threadProc(threadParam)
            : 0;
    }
}
