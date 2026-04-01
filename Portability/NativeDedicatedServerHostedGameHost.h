#pragma once

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThread.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameHostStartResult
    {
        bool threadInvoked = false;
        bool startupReady = false;
        bool sessionSnapshotAvailable = false;
        int startupResult = -1;
        NativeDedicatedServerHostedGameSessionSnapshot sessionSnapshot = {};
    };

    bool SignalNativeDedicatedServerHostedGameHostReady();

    void ResetNativeDedicatedServerHostedGameHostState();

    bool WaitForNativeDedicatedServerHostedGameHostStop(
        DWORD timeoutMs,
        DWORD *outExitCode);

    NativeDedicatedServerHostedGameHostStartResult
    StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks);
}
