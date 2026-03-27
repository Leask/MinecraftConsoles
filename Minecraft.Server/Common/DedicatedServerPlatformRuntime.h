#pragma once

#include <string>

#include <lce_win32/lce_win32.h>

#include "DedicatedServerPlatformState.h"

namespace ServerRuntime
{
    struct DedicatedServerPlatformRuntimeStartResult
    {
        bool ok = false;
        int exitCode = 0;
        std::string errorMessage;
        std::string runtimeName;
        bool headless = false;
    };

    DedicatedServerPlatformRuntimeStartResult
    StartDedicatedServerPlatformRuntime(
        const DedicatedServerPlatformState &platformState);

    void TickDedicatedServerPlatformRuntime();
    void HandleDedicatedServerPlatformActions();
    bool IsDedicatedServerWorldActionIdle(int actionPad);
    void RequestDedicatedServerWorldAutosave(int actionPad);
    bool WaitForDedicatedServerWorldActionIdle(
        int actionPad,
        DWORD timeoutMs);
    void StopDedicatedServerPlatformRuntime();
}
