#pragma once

#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameCoreHooks
    {
        void (*onThreadReady)() = nullptr;
        void (*onThreadStopped)() = nullptr;
    };

    int RunNativeDedicatedServerHostedGameCore(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks);
}
