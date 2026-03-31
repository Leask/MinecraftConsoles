#pragma once

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "NativeDedicatedServerHostedGameThread.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameHostStartResult
    {
        bool threadInvoked = false;
        bool startupReady = false;
        int startupResult = -1;
    };

    bool PrepareNativeDedicatedServerHostedGameHostStartup(
        HANDLE *outStartupReadyEvent);

    HANDLE GetNativeDedicatedServerHostedGameHostStartupReadyEvent();

    bool SignalNativeDedicatedServerHostedGameHostReady();

    void SetNativeDedicatedServerHostedGameHostThreadHandle(
        HANDLE threadHandle);

    void ReleaseNativeDedicatedServerHostedGameHostThreadHandle(
        bool closeHandle);

    void ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();

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
