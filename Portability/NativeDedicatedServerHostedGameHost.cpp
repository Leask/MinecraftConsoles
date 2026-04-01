#include "NativeDedicatedServerHostedGameHost.h"

namespace
{
    HANDLE g_nativeHostedStartupReadyEvent = nullptr;
    HANDLE g_nativeHostedThreadHandle = nullptr;

    void SetNativeDedicatedServerHostedGameHostThreadHandle(
        HANDLE threadHandle);

    void ReleaseNativeDedicatedServerHostedGameHostThreadHandle(
        bool closeHandle);

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
}

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameHostStartResult
    StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
    {
        NativeDedicatedServerHostedGameHostStartResult result = {};
        HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
            threadProc,
            threadParam);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
            return result;
        }

        SetNativeDedicatedServerHostedGameHostThreadHandle(threadHandle);
        result.threadInvoked = true;
        if (WaitForNativeDedicatedServerHostedGameThreadReady(
                g_nativeHostedStartupReadyEvent,
                threadHandle,
                callbacks))
        {
            result.startupReady = true;
            result.startupResult = 0;
            result.sessionSnapshot =
                GetNativeDedicatedServerHostedGameSessionSnapshot();
            result.sessionSnapshotAvailable = true;
            return result;
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedGameThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            threadExitCode = static_cast<DWORD>(-1);
        }

        CloseHandle(threadHandle);
        ReleaseNativeDedicatedServerHostedGameHostThreadHandle(false);
        ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
        result.startupResult = static_cast<int>(threadExitCode);
        return result;
    }

    bool PrepareNativeDedicatedServerHostedGameHostStartup()
    {
        ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
        g_nativeHostedStartupReadyEvent = CreateEvent(
            nullptr,
            TRUE,
            FALSE,
            nullptr);
        return g_nativeHostedStartupReadyEvent != nullptr &&
            g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE;
    }

    bool SignalNativeDedicatedServerHostedGameHostReady()
    {
        return g_nativeHostedStartupReadyEvent != nullptr &&
            g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE &&
            SetEvent(g_nativeHostedStartupReadyEvent) != 0;
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
