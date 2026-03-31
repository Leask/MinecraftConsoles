#include "NativeDedicatedServerHostedGameThread.h"

#include "Minecraft.Server/Common/DedicatedServerSignalState.h"

#include <memory>

namespace
{
    struct NativeDedicatedServerHostedGameThreadContext
    {
        ServerRuntime::DedicatedServerHostedGameThreadProc *threadProc =
            nullptr;
        void *threadParam = nullptr;
    };

    void TickNativeDedicatedServerHostedGameThreadCallbacks(
        const ServerRuntime::NativeDedicatedServerHostedGameThreadCallbacks
            &callbacks)
    {
        if (callbacks.tickPlatformRuntime != nullptr)
        {
            callbacks.tickPlatformRuntime();
        }
        if (callbacks.handlePlatformActions != nullptr)
        {
            callbacks.handlePlatformActions();
        }
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

    bool WaitForNativeDedicatedServerHostedGameThreadReady(
        HANDLE startupReadyEvent,
        HANDLE threadHandle,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
    {
        while (!IsDedicatedServerShutdownRequested())
        {
            if (WaitForSingleObject(startupReadyEvent, 0) == WAIT_OBJECT_0)
            {
                return true;
            }

            if (WaitForSingleObject(threadHandle, 10) == WAIT_OBJECT_0)
            {
                return false;
            }

            TickNativeDedicatedServerHostedGameThreadCallbacks(callbacks);
        }

        return false;
    }

    void PumpNativeDedicatedServerHostedGameThreadUntilExit(
        HANDLE threadHandle,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
    {
        while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT &&
            !IsDedicatedServerShutdownRequested())
        {
            TickNativeDedicatedServerHostedGameThreadCallbacks(callbacks);
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
}
