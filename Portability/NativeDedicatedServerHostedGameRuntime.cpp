#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameCore.h"
#include "NativeDedicatedServerHostedGameHost.h"
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
            (void)SignalNativeDedicatedServerHostedGameHostReady();
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
        ResetNativeDedicatedServerHostedGameHostState();
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

        const bool stopped = WaitForNativeDedicatedServerHostedGameHostStop(
            timeoutMs,
            outExitCode);
        if (stopped)
        {
            FinalizeNativeDedicatedServerHostedThreadStop();
        }
        return stopped;
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

        ActivateDedicatedServerHostedGamePlan(hostedGamePlan);

        NativeDedicatedServerHostedGameThreadCallbacks callbacks = {};
        callbacks.tickPlatformRuntime =
            &TickNativeDedicatedServerHostedGameThreadPlatformRuntime;
        callbacks.handlePlatformActions =
            &HandleNativeDedicatedServerHostedGameThreadPlatformActions;

        if (usePersistentNativeSession)
        {
            const NativeDedicatedServerHostedGamePersistentStartResult
                result =
                    StartPersistentNativeDedicatedServerHostedGameRuntime(
                        hostedGamePlan,
                        threadProc,
                        threadParam,
                        callbacks);
            if (result.ready)
            {
                RecordNativeDedicatedServerHostedThreadSnapshot();
            }
            return CompleteNativeDedicatedServerHostedGameStartup(
                true,
                result.startupResult,
                result.threadInvoked,
                LceGetMonotonicMilliseconds());
        }

        HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
            threadProc,
            threadParam);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            return CompleteNativeDedicatedServerHostedGameStartup(
                false,
                -1,
                false,
                LceGetMonotonicMilliseconds());
        }

        PumpNativeDedicatedServerHostedGameThreadUntilExit(
            threadHandle,
            callbacks);
        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedGameThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            CloseHandle(threadHandle);
            return CompleteNativeDedicatedServerHostedGameStartup(
                false,
                -1,
                true,
                LceGetMonotonicMilliseconds());
        }

        CloseHandle(threadHandle);
        startupResult = static_cast<int>(threadExitCode);
        return CompleteNativeDedicatedServerHostedGameStartup(
            false,
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
