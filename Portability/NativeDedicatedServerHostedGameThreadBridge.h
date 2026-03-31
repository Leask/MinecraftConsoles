#pragma once

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameThread.h"

namespace ServerRuntime
{
    void ProjectNativeDedicatedServerHostedGameThreadSnapshot();

    void FinalizeNativeDedicatedServerHostedGameThreadStop();

    NativeDedicatedServerHostedGameThreadCallbacks
    BuildNativeDedicatedServerHostedGameThreadCallbacks();

    DedicatedServerHostedGameThreadProc
        *GetNativeDedicatedServerHostedGameThreadProc();
}
