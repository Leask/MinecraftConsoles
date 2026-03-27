#pragma once

#include <string>

#include "..\Common\DedicatedServerPlatformState.h"

namespace ServerRuntime
{
    struct DedicatedServerWindowsRuntimeStartResult
    {
        bool ok = false;
        int exitCode = 0;
        std::string errorMessage;
    };

    DedicatedServerWindowsRuntimeStartResult
    StartWindowsDedicatedServerRuntime(
        const DedicatedServerPlatformState &platformState);

    void StopWindowsDedicatedServerRuntime();
}
