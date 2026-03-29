#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"

#include "lce_win32/lce_win32.h"

namespace ServerRuntime
{
    namespace
    {
        struct NativeDedicatedServerHostedGameThreadContext
        {
            DedicatedServerHostedGameThreadProc *threadProc = nullptr;
            void *threadParam = nullptr;
        };

        DWORD WINAPI RunNativeDedicatedServerHostedThreadThunk(
            LPVOID threadParameter)
        {
            NativeDedicatedServerHostedGameThreadContext *context =
                static_cast<NativeDedicatedServerHostedGameThreadContext *>(
                    threadParameter);
            if (context == nullptr || context->threadProc == nullptr)
            {
                return static_cast<DWORD>(-1);
            }

            return static_cast<DWORD>(
                context->threadProc(context->threadParam));
        }

        int RunNativeDedicatedServerHostedGameThread(void *)
        {
            return 0;
        }
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        if (!BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        NativeDedicatedServerHostedGameThreadContext threadContext = {};
        threadContext.threadProc = threadProc;
        threadContext.threadParam = threadParam;
        HANDLE threadHandle = CreateThread(
            nullptr,
            0,
            &RunNativeDedicatedServerHostedThreadThunk,
            &threadContext,
            0,
            nullptr);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            return CompleteDedicatedServerHostedGameRuntimeStartup(-1, false);
        }

        while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT &&
            !IsDedicatedServerShutdownRequested())
        {
            TickDedicatedServerPlatformRuntime();
            HandleDedicatedServerPlatformActions();
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!GetExitCodeThread(threadHandle, &threadExitCode))
        {
            CloseHandle(threadHandle);
            return CompleteDedicatedServerHostedGameRuntimeStartup(-1, true);
        }

        CloseHandle(threadHandle);
        startupResult = static_cast<int>(threadExitCode);
        return CompleteDedicatedServerHostedGameRuntimeStartup(
            startupResult,
            true);
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return &RunNativeDedicatedServerHostedGameThread;
    }
}
