#pragma once

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameThreadRunResult
    {
        bool threadInvoked = false;
        int exitCode = -1;
    };

    struct NativeDedicatedServerHostedGameThreadCallbacks
    {
        void (*tickPlatformRuntime)() = nullptr;
        void (*handlePlatformActions)() = nullptr;
    };

    HANDLE StartNativeDedicatedServerHostedGameThread(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam);

    NativeDedicatedServerHostedGameThreadRunResult
    RunNativeDedicatedServerHostedGameThreadAndReadExitCode(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks);
}
