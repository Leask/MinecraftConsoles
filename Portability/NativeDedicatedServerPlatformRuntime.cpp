#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"

namespace
{
    struct NativeDedicatedServerPlatformRuntimeState
    {
        bool gameplayInstance = false;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool stopSignalValid = false;
        bool saveOnExitEnabled = false;
        unsigned int pendingWorldActionTicks = 0;
        std::uint64_t tickCount = 0;
        std::uint64_t autosaveRequestCount = 0;
        std::uint64_t autosaveCompletionCount = 0;
    };

    NativeDedicatedServerPlatformRuntimeState g_nativeRuntimeState = {};

    bool IsNativeDedicatedServerWorldActionIdleHook(int, void *)
    {
        return g_nativeRuntimeState.pendingWorldActionTicks == 0;
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
}

namespace ServerRuntime
{
    DedicatedServerPlatformRuntimeStartResult
    StartDedicatedServerPlatformRuntime(
        const DedicatedServerPlatformState &)
    {
        DedicatedServerPlatformRuntimeStartResult result = {};
        g_nativeRuntimeState = {};
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
        if (g_nativeRuntimeState.pendingWorldActionTicks > 0)
        {
            --g_nativeRuntimeState.pendingWorldActionTicks;
            if (g_nativeRuntimeState.pendingWorldActionTicks == 0)
            {
                ++g_nativeRuntimeState.autosaveCompletionCount;
            }
        }
    }

    void HandleDedicatedServerPlatformActions()
    {
    }

    bool IsDedicatedServerWorldActionIdle(int)
    {
        return g_nativeRuntimeState.pendingWorldActionTicks == 0;
    }

    void RequestDedicatedServerWorldAutosave(int)
    {
        ++g_nativeRuntimeState.autosaveRequestCount;
        if (g_nativeRuntimeState.pendingWorldActionTicks < 2)
        {
            g_nativeRuntimeState.pendingWorldActionTicks = 2;
        }
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
        return g_nativeRuntimeState.autosaveRequestCount;
    }

    std::uint64_t GetDedicatedServerAutosaveCompletionCount()
    {
        return g_nativeRuntimeState.autosaveCompletionCount;
    }

    void HaltDedicatedServerGameplay()
    {
        g_nativeRuntimeState.gameplayHalted = true;
    }

    void WaitForDedicatedServerStopSignal()
    {
        g_nativeRuntimeState.gameplayInstance = false;
        g_nativeRuntimeState.stopSignalValid = false;
        g_nativeRuntimeState.pendingWorldActionTicks = 0;
    }

    void StopDedicatedServerPlatformRuntime()
    {
        g_nativeRuntimeState = {};
    }
}
