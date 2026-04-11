#pragma once

#include <lce_win32/lce_win32.h>

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameHostState();

    bool SignalNativeDedicatedServerHostedGameHostReady();

    bool WaitForNativeDedicatedServerHostedGameHostStop(
        DWORD timeoutMs,
        DWORD *outExitCode);
}
