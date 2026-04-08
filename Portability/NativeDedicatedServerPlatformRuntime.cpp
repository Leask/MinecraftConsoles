#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerAutosaveTracker.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "NativeDedicatedServerHostedGameWorker.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "lce_time/lce_time.h"

#include <algorithm>

namespace ServerRuntime
{
    bool IsNativeDedicatedServerHostedGameSessionRunning();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    void ResetNativeDedicatedServerHostedGameSessionState();

    void ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
        std::uint64_t nowMs = 0);
    void RequestNativeDedicatedServerHostedGameSessionAutosave(
        unsigned int workTicks,
        std::uint64_t nowMs = 0);
    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionHaltSequence(
        bool requestAutosaveFirst,
        unsigned int autosaveWorkTicks,
        std::uint64_t nowMs = 0);
    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);

    void ObserveNativeDedicatedServerHostedGameSessionAutosavesAndProject(
        std::uint64_t autosaveCompletions,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionPlatformRuntimeStateAndProject(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount,
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t nowMs = 0);
}

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

    void RefreshNativeDedicatedServerRuntimeProjection(
        std::uint64_t nowMs = 0)
    {
        const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot =
                ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
        ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionPlatformRuntimeStateAndProject(
            ServerRuntime::GetDedicatedServerAutosaveRequestCount(),
            ServerRuntime::GetDedicatedServerPlatformTickCount(),
            sessionSnapshot.gameplayLoopIterations,
            g_nativeRuntimeState.appShutdownRequested,
            g_nativeRuntimeState.gameplayHalted,
            g_nativeRuntimeState.stopSignalValid,
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
            ObserveNativeDedicatedServerHostedGameSessionAutosavesAndProject(
                GetDedicatedServerAutosaveCompletionCount(),
                LceGetMonotonicMilliseconds());
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
        if (IsNativeDedicatedServerHostedGameSessionRunning())
        {
            RequestNativeDedicatedServerHostedGameSessionAutosave(
                2,
                LceGetMonotonicMilliseconds());
        }
        else
        {
            MarkDedicatedServerAutosaveTrackerRequested();
            if (g_nativeRuntimeState.fallbackWorldActionTicks < 2)
            {
                g_nativeRuntimeState.fallbackWorldActionTicks = 2;
            }
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
        const std::uint64_t trackerCompletions =
            GetDedicatedServerAutosaveTrackerCompletionCount();
        if (!IsNativeDedicatedServerHostedGameSessionRunning())
        {
            return trackerCompletions;
        }

        return std::max(
            trackerCompletions,
            GetNativeDedicatedServerHostedGameSessionSnapshot()
                .processedAutosaveCommands);
    }

    void HaltDedicatedServerGameplay()
    {
        if (IsNativeDedicatedServerHostedGameSessionRunning())
        {
            EnqueueNativeDedicatedServerHostedGameSessionHaltSequence(
                g_nativeRuntimeState.saveOnExitEnabled,
                2,
                LceGetMonotonicMilliseconds());
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
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
            LceGetMonotonicMilliseconds());
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
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
            LceGetMonotonicMilliseconds());
        ResetDedicatedServerAutosaveTracker();
        g_nativeRuntimeState = {};
    }
}
