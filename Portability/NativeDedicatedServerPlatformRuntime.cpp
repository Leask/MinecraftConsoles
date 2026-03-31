#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerAutosaveTracker.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "NativeDedicatedServerHostedGameWorker.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "lce_time/lce_time.h"

namespace
{
    struct NativeDedicatedServerPlatformRuntimeState
    {
        bool gameplayInstance = false;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool stopSignalValid = false;
        bool saveOnExitEnabled = false;
        unsigned int fallbackWorldActionTicks = 0;
        std::uint64_t tickCount = 0;
    };

    NativeDedicatedServerPlatformRuntimeState g_nativeRuntimeState = {};

    bool IsNativeDedicatedServerWorldActionIdleHook(int, void *)
    {
        return ServerRuntime::IsDedicatedServerWorldActionIdle(0);
    }

    bool IsNativeDedicatedServerGameplayHaltedHook(void *)
    {
        return g_nativeRuntimeState.gameplayHalted;
    }

    void TickNativeDedicatedServerPlatformRuntimeHook(void *)
    {
        ServerRuntime::TickDedicatedServerPlatformRuntime();
    }

    void HandleNativeDedicatedServerPlatformActionsHook(void *)
    {
        ServerRuntime::HandleDedicatedServerPlatformActions();
    }

    void RefreshNativeDedicatedServerWorkerProjection()
    {
        const ServerRuntime::NativeDedicatedServerHostedGameWorkerSnapshot
            workerSnapshot =
                ServerRuntime::GetNativeDedicatedServerHostedGameWorkerSnapshot();
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionWorkerSnapshot(
            workerSnapshot);
        ServerRuntime::ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot();
    }

    void RefreshNativeDedicatedServerRuntimeProjection(
        std::uint64_t nowMs = 0)
    {
        const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot =
                ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionPlatformState(
            ServerRuntime::GetDedicatedServerAutosaveRequestCount(),
            ServerRuntime::GetDedicatedServerPlatformTickCount());
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
            sessionSnapshot.gameplayLoopIterations,
            g_nativeRuntimeState.appShutdownRequested,
            g_nativeRuntimeState.gameplayHalted,
            g_nativeRuntimeState.stopSignalValid);
        ServerRuntime::ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
            nowMs);
    }
}

namespace ServerRuntime
{
    DedicatedServerPlatformRuntimeStartResult
    StartDedicatedServerPlatformRuntime(
        const DedicatedServerPlatformState &)
    {
        DedicatedServerPlatformRuntimeStartResult result = {};
        g_nativeRuntimeState = {};
        ResetDedicatedServerAutosaveTracker();
        ResetNativeDedicatedServerHostedGameWorkerState();
        ResetNativeDedicatedServerHostedGameSessionState();
        g_nativeRuntimeState.gameplayInstance = true;
        g_nativeRuntimeState.stopSignalValid = true;
        result.ok = true;
        result.exitCode = 0;
        result.runtimeName = "NativeDesktopStub";
        result.headless = true;
        return result;
    }

    DedicatedServerAppSessionApplyResult ApplyDedicatedServerAppSessionPlan(
        const DedicatedServerAppSessionPlan &appSessionPlan)
    {
        DedicatedServerAppSessionApplyResult result = {};
        result.applied = true;
        result.initializedGameSettings =
            appSessionPlan.shouldInitGameSettings;
        result.appliedWorldSize = appSessionPlan.shouldApplyWorldSize;
        result.saveDisabled = appSessionPlan.saveDisabled;
        return result;
    }

    void TickDedicatedServerPlatformRuntime()
    {
        ++g_nativeRuntimeState.tickCount;
        const bool sessionRunning =
            IsNativeDedicatedServerHostedGameSessionRunning();
        if (!sessionRunning &&
            g_nativeRuntimeState.fallbackWorldActionTicks > 0)
        {
            --g_nativeRuntimeState.fallbackWorldActionTicks;
        }
        if (!sessionRunning)
        {
            UpdateDedicatedServerAutosaveTracker(
                IsDedicatedServerWorldActionIdle(0));
            ObserveNativeDedicatedServerHostedGameSessionAutosaves(
                GetDedicatedServerAutosaveCompletionCount());
        }
        RefreshNativeDedicatedServerRuntimeProjection(
            LceGetMonotonicMilliseconds());
    }

    void HandleDedicatedServerPlatformActions()
    {
    }

    bool IsDedicatedServerWorldActionIdle(int)
    {
        if (IsNativeDedicatedServerHostedGameSessionRunning())
        {
            return GetNativeDedicatedServerHostedGameSessionSnapshot()
                .worldActionIdle;
        }

        return g_nativeRuntimeState.fallbackWorldActionTicks == 0;
    }

    void RequestDedicatedServerWorldAutosave(int)
    {
        MarkDedicatedServerAutosaveTrackerRequested();
        if (IsNativeDedicatedServerHostedGameSessionRunning())
        {
            RequestNativeDedicatedServerHostedGameWorkerAutosave(2);
            RefreshNativeDedicatedServerWorkerProjection();
        }
        else if (g_nativeRuntimeState.fallbackWorldActionTicks < 2)
        {
            g_nativeRuntimeState.fallbackWorldActionTicks = 2;
        }
        RefreshNativeDedicatedServerRuntimeProjection(
            LceGetMonotonicMilliseconds());
    }

    bool WaitForDedicatedServerWorldActionIdle(
        int actionPad,
        DWORD timeoutMs)
    {
        DedicatedServerWorldActionWaitHooks hooks = {};
        hooks.isIdleProc = &IsNativeDedicatedServerWorldActionIdleHook;
        hooks.haltProc = &IsNativeDedicatedServerGameplayHaltedHook;
        hooks.tickProc = &TickNativeDedicatedServerPlatformRuntimeHook;
        hooks.handleActionsProc = &HandleNativeDedicatedServerPlatformActionsHook;
        return WaitForDedicatedServerWorldActionIdleWithHooks(
            actionPad,
            timeoutMs,
            hooks);
    }

    bool HasDedicatedServerGameplayInstance()
    {
        return g_nativeRuntimeState.gameplayInstance;
    }

    bool IsDedicatedServerAppShutdownRequested()
    {
        return g_nativeRuntimeState.appShutdownRequested;
    }

    void SetDedicatedServerAppShutdownRequested(bool shutdownRequested)
    {
        g_nativeRuntimeState.appShutdownRequested = shutdownRequested;
        RefreshNativeDedicatedServerRuntimeProjection(
            LceGetMonotonicMilliseconds());
    }

    bool IsDedicatedServerGameplayHalted()
    {
        return g_nativeRuntimeState.gameplayHalted;
    }

    bool IsDedicatedServerStopSignalValid()
    {
        return g_nativeRuntimeState.stopSignalValid;
    }

    void EnableDedicatedServerSaveOnExit()
    {
        g_nativeRuntimeState.saveOnExitEnabled = true;
    }

    std::uint64_t GetDedicatedServerPlatformTickCount()
    {
        return g_nativeRuntimeState.tickCount;
    }

    std::uint64_t GetDedicatedServerAutosaveRequestCount()
    {
        return GetDedicatedServerAutosaveTrackerRequestCount();
    }

    std::uint64_t GetDedicatedServerAutosaveCompletionCount()
    {
        return GetDedicatedServerAutosaveTrackerCompletionCount();
    }

    void HaltDedicatedServerGameplay()
    {
        if (IsNativeDedicatedServerHostedGameSessionRunning())
        {
            EnqueueNativeDedicatedServerHostedGameWorkerHaltSequence(
                g_nativeRuntimeState.saveOnExitEnabled,
                2);
            RefreshNativeDedicatedServerWorkerProjection();
            g_nativeRuntimeState.gameplayHalted = true;
            RefreshNativeDedicatedServerRuntimeProjection(
                LceGetMonotonicMilliseconds());
            return;
        }

        if (g_nativeRuntimeState.saveOnExitEnabled)
        {
            RequestDedicatedServerWorldAutosave(0);
        }
        g_nativeRuntimeState.gameplayHalted = true;
        RefreshNativeDedicatedServerRuntimeProjection(
            LceGetMonotonicMilliseconds());
    }

    void WaitForDedicatedServerStopSignal()
    {
        WaitForNativeDedicatedServerHostedGameSessionStop(INFINITE);
        g_nativeRuntimeState.gameplayInstance = false;
        g_nativeRuntimeState.stopSignalValid = false;
        g_nativeRuntimeState.fallbackWorldActionTicks = 0;
        ClearNativeDedicatedServerHostedGameWorkerState();
        RefreshNativeDedicatedServerWorkerProjection();
        RefreshNativeDedicatedServerRuntimeProjection(
            LceGetMonotonicMilliseconds());
    }

    void StopDedicatedServerPlatformRuntime()
    {
        if (!WaitForNativeDedicatedServerHostedGameSessionStop(0))
        {
            g_nativeRuntimeState.appShutdownRequested = true;
            RefreshNativeDedicatedServerRuntimeProjection(
                LceGetMonotonicMilliseconds());
            WaitForNativeDedicatedServerHostedGameSessionStop(INFINITE);
        }
        ClearNativeDedicatedServerHostedGameWorkerState();
        RefreshNativeDedicatedServerWorkerProjection();
        ResetDedicatedServerAutosaveTracker();
        g_nativeRuntimeState = {};
    }
}
