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
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionWorkerState(
            workerSnapshot.pendingWorldActionTicks,
            workerSnapshot.pendingAutosaveCommands,
            workerSnapshot.pendingSaveCommands,
            workerSnapshot.pendingStopCommands,
            workerSnapshot.pendingHaltCommands,
            workerSnapshot.workerTickCount,
            workerSnapshot.completedWorldActions,
            workerSnapshot.processedAutosaveCommands,
            workerSnapshot.processedSaveCommands,
            workerSnapshot.processedStopCommands,
            workerSnapshot.processedHaltCommands,
            workerSnapshot.lastQueuedCommandId,
            workerSnapshot.activeCommandId,
            workerSnapshot.activeCommandTicksRemaining,
            workerSnapshot.activeCommandKind,
            workerSnapshot.lastProcessedCommandId,
            workerSnapshot.lastProcessedCommandKind);
        const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot =
                ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
        ServerRuntime::RecordDedicatedServerHostedGameRuntimeWorkerState(
            sessionSnapshot.workerPendingWorldActionTicks,
            sessionSnapshot.workerPendingAutosaveCommands,
            sessionSnapshot.workerPendingSaveCommands,
            sessionSnapshot.workerPendingStopCommands,
            sessionSnapshot.workerPendingHaltCommands,
            sessionSnapshot.workerTickCount,
            sessionSnapshot.completedWorkerActions,
            sessionSnapshot.processedAutosaveCommands,
            sessionSnapshot.processedSaveCommands,
            sessionSnapshot.processedStopCommands,
            sessionSnapshot.processedHaltCommands,
            sessionSnapshot.lastQueuedCommandId,
            sessionSnapshot.activeCommandId,
            sessionSnapshot.activeCommandTicksRemaining,
            (unsigned int)sessionSnapshot.activeCommandKind,
            sessionSnapshot.lastProcessedCommandId,
            (unsigned int)sessionSnapshot.lastProcessedCommandKind);
        ServerRuntime::RecordDedicatedServerHostedGameRuntimeCoreState(
            sessionSnapshot.saveGeneration,
            sessionSnapshot.stateChecksum);
        ServerRuntime::RecordDedicatedServerHostedGameRuntimeCoreLifecycle(
            sessionSnapshot.active,
            (ServerRuntime::EDedicatedServerHostedGameRuntimePhase)
                sessionSnapshot.runtimePhase);
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
        if (!IsNativeDedicatedServerHostedGameSessionRunning() &&
            g_nativeRuntimeState.fallbackWorldActionTicks > 0)
        {
            --g_nativeRuntimeState.fallbackWorldActionTicks;
        }
        UpdateDedicatedServerAutosaveTracker(
            IsDedicatedServerWorldActionIdle(0));
        ObserveNativeDedicatedServerHostedGameSessionAutosaves(
            GetDedicatedServerAutosaveCompletionCount());
        ObserveNativeDedicatedServerHostedGameSessionPlatformState(
            GetDedicatedServerAutosaveRequestCount(),
            GetDedicatedServerPlatformTickCount());
        const NativeDedicatedServerHostedGameSessionSnapshot sessionSnapshot =
            GetNativeDedicatedServerHostedGameSessionSnapshot();
        UpdateDedicatedServerHostedGameRuntimeSessionState(
            sessionSnapshot.acceptedConnections,
            sessionSnapshot.remoteCommands,
            sessionSnapshot.autosaveRequests,
            sessionSnapshot.observedAutosaveCompletions,
            sessionSnapshot.platformTickCount,
            sessionSnapshot.worldActionIdle,
            sessionSnapshot.appShutdownRequested,
            sessionSnapshot.gameplayHalted,
            sessionSnapshot.stopSignalValid,
            LceGetMonotonicMilliseconds());
        RecordDedicatedServerHostedGameRuntimeCoreState(
            sessionSnapshot.saveGeneration,
            sessionSnapshot.stateChecksum);
        RecordDedicatedServerHostedGameRuntimeThreadState(
            sessionSnapshot.hostedThreadActive,
            sessionSnapshot.hostedThreadTicks);
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
        ObserveNativeDedicatedServerHostedGameSessionPlatformState(
            GetDedicatedServerAutosaveRequestCount(),
            GetDedicatedServerPlatformTickCount());
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
            ObserveNativeDedicatedServerHostedGameSessionPlatformState(
                GetDedicatedServerAutosaveRequestCount(),
                GetDedicatedServerPlatformTickCount());
            return;
        }

        if (g_nativeRuntimeState.saveOnExitEnabled)
        {
            RequestDedicatedServerWorldAutosave(0);
        }
        g_nativeRuntimeState.gameplayHalted = true;
    }

    void WaitForDedicatedServerStopSignal()
    {
        WaitForNativeDedicatedServerHostedGameSessionStop(INFINITE);
        g_nativeRuntimeState.gameplayInstance = false;
        g_nativeRuntimeState.stopSignalValid = false;
        g_nativeRuntimeState.fallbackWorldActionTicks = 0;
        ClearNativeDedicatedServerHostedGameWorkerState();
        RecordDedicatedServerHostedGameRuntimeThreadState(
            IsNativeDedicatedServerHostedGameSessionRunning(),
            GetNativeDedicatedServerHostedGameSessionThreadTicks());
    }

    void StopDedicatedServerPlatformRuntime()
    {
        if (!WaitForNativeDedicatedServerHostedGameSessionStop(0))
        {
            g_nativeRuntimeState.appShutdownRequested = true;
            WaitForNativeDedicatedServerHostedGameSessionStop(INFINITE);
        }
        ClearNativeDedicatedServerHostedGameWorkerState();
        ResetDedicatedServerAutosaveTracker();
        g_nativeRuntimeState = {};
    }
}
