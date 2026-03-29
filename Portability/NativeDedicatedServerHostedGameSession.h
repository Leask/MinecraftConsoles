#pragma once

#include <cstdint>

#include <lce_win32/lce_win32.h>

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameSessionState();

    bool IsNativeDedicatedServerHostedGameSessionRunning();

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks();

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);
}
