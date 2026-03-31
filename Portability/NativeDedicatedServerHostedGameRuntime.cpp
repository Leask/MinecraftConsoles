#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameCore.h"
#include "NativeDedicatedServerHostedGameStartup.h"
#include "NativeDedicatedServerHostedGameThread.h"
#include "NativeDedicatedServerHostedGameWorker.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include "lce_win32/lce_win32.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameSessionCoreState();

    void ActivateDedicatedServerHostedGamePlan(
        const DedicatedServerHostedGamePlan &)
    {
    }

    namespace
    {
        HANDLE g_nativeHostedStartupReadyEvent = nullptr;
        HANDLE g_nativeHostedThreadHandle = nullptr;

        void CloseNativeDedicatedServerHostedStartupReadyEvent()
        {
            if (g_nativeHostedStartupReadyEvent != nullptr &&
                g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE)
            {
                CloseHandle(g_nativeHostedStartupReadyEvent);
                g_nativeHostedStartupReadyEvent = nullptr;
            }
        }

        void RecordNativeDedicatedServerHostedThreadSnapshot()
        {
            ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot();
        }

        void SyncNativeDedicatedServerHostedThreadState(bool running)
        {
            ObserveNativeDedicatedServerHostedGameSessionThreadStateAndWorkerProject(
                running,
                GetNativeDedicatedServerHostedGameSessionThreadTicks(),
                LceGetMonotonicMilliseconds());
        }

        void SignalNativeDedicatedServerHostedThreadReady()
        {
            SyncNativeDedicatedServerHostedThreadState(true);
            if (g_nativeHostedStartupReadyEvent != nullptr &&
                g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE)
            {
                SetEvent(g_nativeHostedStartupReadyEvent);
            }
        }

        void FinalizeNativeDedicatedServerHostedThreadStop()
        {
            SyncNativeDedicatedServerHostedThreadState(false);
        }

        void TickNativeDedicatedServerHostedGameThreadPlatformRuntime()
        {
            TickDedicatedServerPlatformRuntime();
        }

        void HandleNativeDedicatedServerHostedGameThreadPlatformActions()
        {
            HandleDedicatedServerPlatformActions();
        }

        int RunNativeDedicatedServerHostedGameThread(void *threadParam)
        {
            NativeDedicatedServerHostedGameCoreHooks hooks = {};
            hooks.onThreadReady =
                &SignalNativeDedicatedServerHostedThreadReady;
            hooks.onThreadStopped =
                &FinalizeNativeDedicatedServerHostedThreadStop;
            return RunNativeDedicatedServerHostedGameCore(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hooks);
        }
    }

    void ResetNativeDedicatedServerHostedGameSessionState()
    {
        if (g_nativeHostedThreadHandle != nullptr &&
            g_nativeHostedThreadHandle != INVALID_HANDLE_VALUE)
        {
            if (WaitForSingleObject(g_nativeHostedThreadHandle, 0) ==
                WAIT_OBJECT_0)
            {
                CloseHandle(g_nativeHostedThreadHandle);
                g_nativeHostedThreadHandle = nullptr;
            }
        }

        CloseNativeDedicatedServerHostedStartupReadyEvent();
        ResetNativeDedicatedServerHostedGameWorkerState();
        ResetNativeDedicatedServerHostedGameSessionCoreState();
        RecordNativeDedicatedServerHostedThreadSnapshot();
    }

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
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
            CloseNativeDedicatedServerHostedStartupReadyEvent();
            FinalizeNativeDedicatedServerHostedThreadStop();
            return true;
        }

        const DWORD waitResult = WaitForSingleObject(
            g_nativeHostedThreadHandle,
            timeoutMs);
        if (waitResult != WAIT_OBJECT_0)
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

        CloseHandle(g_nativeHostedThreadHandle);
        g_nativeHostedThreadHandle = nullptr;
        CloseNativeDedicatedServerHostedStartupReadyEvent();
        FinalizeNativeDedicatedServerHostedThreadStop();
        return true;
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        const bool usePersistentNativeSession =
            threadProc == &RunNativeDedicatedServerHostedGameThread;
        if (!usePersistentNativeSession &&
            !BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        if (!PrepareNativeDedicatedServerHostedGameStartup(
                usePersistentNativeSession,
                &g_nativeHostedStartupReadyEvent))
        {
            CloseNativeDedicatedServerHostedStartupReadyEvent();
            return CompleteNativeDedicatedServerHostedGameStartup(
                usePersistentNativeSession,
                -1,
                false,
                LceGetMonotonicMilliseconds());
        }

        ActivateDedicatedServerHostedGamePlan(hostedGamePlan);

        if (usePersistentNativeSession)
        {
            PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hostedGamePlan);
        }

        NativeDedicatedServerHostedGameThreadCallbacks callbacks = {};
        callbacks.tickPlatformRuntime =
            &TickNativeDedicatedServerHostedGameThreadPlatformRuntime;
        callbacks.handlePlatformActions =
            &HandleNativeDedicatedServerHostedGameThreadPlatformActions;
        HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
            threadProc,
            threadParam);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            if (usePersistentNativeSession)
            {
                CloseNativeDedicatedServerHostedStartupReadyEvent();
            }
            return CompleteNativeDedicatedServerHostedGameStartup(
                usePersistentNativeSession,
                -1,
                false,
                LceGetMonotonicMilliseconds());
        }

        if (usePersistentNativeSession)
        {
            g_nativeHostedThreadHandle = threadHandle;
            if (WaitForNativeDedicatedServerHostedGameThreadReady(
                    g_nativeHostedStartupReadyEvent,
                    threadHandle,
                    callbacks))
            {
                RecordNativeDedicatedServerHostedThreadSnapshot();
                return CompleteNativeDedicatedServerHostedGameStartup(
                    usePersistentNativeSession,
                    0,
                    true,
                    LceGetMonotonicMilliseconds());
            }
        }
        else
        {
            PumpNativeDedicatedServerHostedGameThreadUntilExit(
                threadHandle,
                callbacks);
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedGameThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            CloseHandle(threadHandle);
            if (usePersistentNativeSession)
            {
                g_nativeHostedThreadHandle = nullptr;
                CloseNativeDedicatedServerHostedStartupReadyEvent();
            }
            return CompleteNativeDedicatedServerHostedGameStartup(
                usePersistentNativeSession,
                -1,
                true,
                LceGetMonotonicMilliseconds());
        }

        CloseHandle(threadHandle);
        if (usePersistentNativeSession)
        {
            g_nativeHostedThreadHandle = nullptr;
            CloseNativeDedicatedServerHostedStartupReadyEvent();
        }
        startupResult = static_cast<int>(threadExitCode);
        return CompleteNativeDedicatedServerHostedGameStartup(
            usePersistentNativeSession,
            startupResult,
            true,
            LceGetMonotonicMilliseconds());
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return &RunNativeDedicatedServerHostedGameThread;
    }
}
