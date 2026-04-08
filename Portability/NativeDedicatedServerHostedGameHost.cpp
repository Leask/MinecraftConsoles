#include "NativeDedicatedServerHostedGameHost.h"

#include "Minecraft.Server/Common/DedicatedServerHostedGameThreadProc.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"

namespace ServerRuntime
{
    HANDLE StartNativeDedicatedServerHostedGameThread(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam);
}

namespace
{
    HANDLE g_nativeHostedStartupReadyEvent = nullptr;
    HANDLE g_nativeHostedThreadHandle = nullptr;

    void ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();

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

    void ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent()
    {
        if (g_nativeHostedStartupReadyEvent != nullptr &&
            g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_nativeHostedStartupReadyEvent);
        }

        g_nativeHostedStartupReadyEvent = nullptr;
    }

    bool WaitForNativeDedicatedServerHostedGameHostThreadReady(
        HANDLE threadHandle)
    {
        while (!ServerRuntime::IsDedicatedServerShutdownRequested())
        {
            if (WaitForSingleObject(g_nativeHostedStartupReadyEvent, 0) ==
                WAIT_OBJECT_0)
            {
                return true;
            }

            if (WaitForSingleObject(threadHandle, 10) == WAIT_OBJECT_0)
            {
                return false;
            }

            ServerRuntime::TickDedicatedServerPlatformRuntime();
            ServerRuntime::HandleDedicatedServerPlatformActions();
        }

        return false;
    }

    bool TryReadNativeDedicatedServerHostedGameHostThreadExitCode(
        HANDLE threadHandle,
        DWORD *outExitCode)
    {
        if (outExitCode == nullptr)
        {
            return false;
        }

        *outExitCode = static_cast<DWORD>(-1);
        return GetExitCodeThread(threadHandle, outExitCode);
    }
}

namespace ServerRuntime
{
    static bool PrepareNativeDedicatedServerHostedGameHostStartup();

    int StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        bool *outThreadInvoked)
    {
        if (outThreadInvoked != nullptr)
        {
            *outThreadInvoked = false;
        }

        if (!PrepareNativeDedicatedServerHostedGameHostStartup())
        {
            return -1;
        }

        HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
            threadProc,
            threadParam);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
            return -1;
        }

        SetNativeDedicatedServerHostedGameHostThreadHandle(threadHandle);
        if (outThreadInvoked != nullptr)
        {
            *outThreadInvoked = true;
        }
        if (WaitForNativeDedicatedServerHostedGameHostThreadReady(
                threadHandle))
        {
            return 0;
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedGameHostThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            threadExitCode = static_cast<DWORD>(-1);
        }

        CloseHandle(threadHandle);
        ReleaseNativeDedicatedServerHostedGameHostThreadHandle(false);
        ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
        return static_cast<int>(threadExitCode);
    }

    static bool PrepareNativeDedicatedServerHostedGameHostStartup()
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
