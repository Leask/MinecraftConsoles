#pragma once

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameHost.h"

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameHostStartResult
    StartNativeDedicatedServerHostedGameRuntimePath(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam);
}
