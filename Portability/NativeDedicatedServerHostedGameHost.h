#pragma once

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameSessionSnapshot;

    bool SignalNativeDedicatedServerHostedGameHostReady();

    void ResetNativeDedicatedServerHostedGameHostState();

    bool WaitForNativeDedicatedServerHostedGameHostStop(
        DWORD timeoutMs,
        DWORD *outExitCode);

    int
    StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        bool *outThreadInvoked,
        NativeDedicatedServerHostedGameSessionSnapshot *outSessionSnapshot,
        bool *outSessionSnapshotAvailable);
}
