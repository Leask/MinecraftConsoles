#pragma once

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/DedicatedServerHostedGameThreadProc.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameHostState();

    bool WaitForNativeDedicatedServerHostedGameHostStop(
        DWORD timeoutMs,
        DWORD *outExitCode);

    int StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        bool *outThreadInvoked);
}
