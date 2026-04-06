#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameThreadProc.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"

#include <lce_win32/lce_win32.h>

#include <memory>

namespace
{
    struct NativeDedicatedServerHostedGameThreadContext
    {
        ServerRuntime::DedicatedServerHostedGameThreadProc *threadProc =
            nullptr;
        void *threadParam = nullptr;
    };

    void PumpNativeDedicatedServerHostedGameThreadUntilExit(HANDLE threadHandle)
    {
        while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT &&
            !ServerRuntime::IsDedicatedServerShutdownRequested())
        {
            ServerRuntime::TickDedicatedServerPlatformRuntime();
            ServerRuntime::HandleDedicatedServerPlatformActions();
        }
    }

    bool TryReadNativeDedicatedServerHostedGameThreadExitCode(
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

    DWORD WINAPI RunNativeDedicatedServerHostedGameThreadThunk(
        LPVOID threadParameter)
    {
        std::unique_ptr<NativeDedicatedServerHostedGameThreadContext> context(
            static_cast<NativeDedicatedServerHostedGameThreadContext *>(
                threadParameter));
        if (context == nullptr || context->threadProc == nullptr)
        {
            return static_cast<DWORD>(-1);
        }

        return static_cast<DWORD>(
            context->threadProc(context->threadParam));
    }
}

namespace ServerRuntime
{
    HANDLE StartNativeDedicatedServerHostedGameThread(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        std::unique_ptr<NativeDedicatedServerHostedGameThreadContext> context(
            new NativeDedicatedServerHostedGameThreadContext{});
        context->threadProc = threadProc;
        context->threadParam = threadParam;
        HANDLE threadHandle = CreateThread(
            nullptr,
            0,
            &RunNativeDedicatedServerHostedGameThreadThunk,
            context.get(),
            0,
            nullptr);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            return threadHandle;
        }

        (void)context.release();
        return threadHandle;
    }

    int RunNativeDedicatedServerHostedGameTransientThread(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        bool *outThreadInvoked)
    {
        if (outThreadInvoked != nullptr)
        {
            *outThreadInvoked = false;
        }

        HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
            threadProc,
            threadParam);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            return -1;
        }

        if (outThreadInvoked != nullptr)
        {
            *outThreadInvoked = true;
        }

        PumpNativeDedicatedServerHostedGameThreadUntilExit(threadHandle);
        WaitForSingleObject(threadHandle, INFINITE);

        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedGameThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            threadExitCode = static_cast<DWORD>(-1);
        }

        CloseHandle(threadHandle);
        return static_cast<int>(threadExitCode);
    }
}
