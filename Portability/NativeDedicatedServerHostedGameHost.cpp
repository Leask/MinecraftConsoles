#include "NativeDedicatedServerHostedGameHost.h"

namespace
{
    HANDLE g_nativeHostedStartupReadyEvent = nullptr;
    HANDLE g_nativeHostedThreadHandle = nullptr;
}

namespace ServerRuntime
{
    bool PrepareNativeDedicatedServerHostedGameHostStartup(
        HANDLE *outStartupReadyEvent)
    {
        ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
        g_nativeHostedStartupReadyEvent = CreateEvent(
            nullptr,
            TRUE,
            FALSE,
            nullptr);
        if (outStartupReadyEvent != nullptr)
        {
            *outStartupReadyEvent = g_nativeHostedStartupReadyEvent;
        }

        return g_nativeHostedStartupReadyEvent != nullptr &&
            g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE;
    }

    HANDLE GetNativeDedicatedServerHostedGameHostStartupReadyEvent()
    {
        return g_nativeHostedStartupReadyEvent;
    }

    bool SignalNativeDedicatedServerHostedGameHostReady()
    {
        return g_nativeHostedStartupReadyEvent != nullptr &&
            g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE &&
            SetEvent(g_nativeHostedStartupReadyEvent) != 0;
    }

    void SetNativeDedicatedServerHostedGameHostThreadHandle(
        HANDLE threadHandle)
    {
        g_nativeHostedThreadHandle = threadHandle;
    }

    void ReleaseNativeDedicatedServerHostedGameHostThreadHandle(
        bool closeHandle)
    {
        if (closeHandle &&
            g_nativeHostedThreadHandle != nullptr &&
            g_nativeHostedThreadHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_nativeHostedThreadHandle);
        }

        g_nativeHostedThreadHandle = nullptr;
    }

    void ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent()
    {
        if (g_nativeHostedStartupReadyEvent != nullptr &&
            g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_nativeHostedStartupReadyEvent);
        }

        g_nativeHostedStartupReadyEvent = nullptr;
    }

    void ResetNativeDedicatedServerHostedGameHostState()
    {
        if (g_nativeHostedThreadHandle != nullptr &&
            g_nativeHostedThreadHandle != INVALID_HANDLE_VALUE &&
            WaitForSingleObject(g_nativeHostedThreadHandle, 0) ==
                WAIT_OBJECT_0)
        {
            ReleaseNativeDedicatedServerHostedGameHostThreadHandle(true);
        }

        ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
    }

    bool WaitForNativeDedicatedServerHostedGameHostStop(
        DWORD timeoutMs,
        DWORD *outExitCode)
    {
        if (outExitCode != nullptr)
        {
            *outExitCode = 0;
        }

        if (g_nativeHostedThreadHandle == nullptr ||
            g_nativeHostedThreadHandle == INVALID_HANDLE_VALUE)
        {
            ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
            return true;
        }

        if (WaitForSingleObject(g_nativeHostedThreadHandle, timeoutMs) !=
            WAIT_OBJECT_0)
        {
            return false;
        }

        DWORD threadExitCode = 0;
        if (!GetExitCodeThread(g_nativeHostedThreadHandle, &threadExitCode))
        {
            threadExitCode = static_cast<DWORD>(-1);
        }

        if (outExitCode != nullptr)
        {
            *outExitCode = threadExitCode;
        }

        ReleaseNativeDedicatedServerHostedGameHostThreadHandle(true);
        ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
        return true;
    }
}
