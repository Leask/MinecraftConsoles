#pragma once

#include <lce_abi/lce_abi.h>

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameHostState();

    bool SignalNativeDedicatedServerHostedGameHostReady();

    bool WaitForNativeDedicatedServerHostedGameHostStop(
        DWORD timeoutMs,
        DWORD *outExitCode);
}
