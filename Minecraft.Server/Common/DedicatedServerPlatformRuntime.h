#pragma once

#include <string>

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
    void StopDedicatedServerPlatformRuntime();
}
