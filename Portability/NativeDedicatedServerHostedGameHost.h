#pragma once

#include <lce_win32/lce_win32.h>

namespace ServerRuntime
{
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
}
