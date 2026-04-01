#pragma once

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameThread.h"

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameThreadCallbacks
    BuildNativeDedicatedServerHostedGameThreadCallbacks();

    DedicatedServerHostedGameThreadProc
        *GetNativeDedicatedServerHostedGameThreadProc();
}
