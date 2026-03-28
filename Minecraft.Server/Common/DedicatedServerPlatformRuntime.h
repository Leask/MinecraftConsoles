#pragma once

#include <string>

#include <lce_win32/lce_win32.h>
#include <lce_time/lce_time.h>

#include "DedicatedServerPlatformState.h"
#include "DedicatedServerSessionConfig.h"

namespace ServerRuntime
{
    struct DedicatedServerPlatformRuntimeStartResult
    {
        bool ok = false;
        int exitCode = 0;
        std::string errorMessage;
        std::string runtimeName;
        bool headless = false;
    };

    struct DedicatedServerAppSessionApplyResult
    {
        bool applied = false;
        bool initializedGameSettings = false;
        bool appliedWorldSize = false;
        bool saveDisabled = false;
    };

    typedef bool (*DedicatedServerWorldActionIdleProc)(
        int actionPad,
        void *context);
    typedef bool (*DedicatedServerWorldActionHaltProc)(void *context);
    typedef void (*DedicatedServerPlatformWorkProc)(void *context);

    struct DedicatedServerWorldActionWaitHooks
    {
        DedicatedServerWorldActionIdleProc isIdleProc = nullptr;
        void *idleContext = nullptr;
        DedicatedServerWorldActionHaltProc haltProc = nullptr;
        void *haltContext = nullptr;
        DedicatedServerPlatformWorkProc tickProc = nullptr;
        void *tickContext = nullptr;
        DedicatedServerPlatformWorkProc handleActionsProc = nullptr;
        void *handleActionsContext = nullptr;
    };

    inline bool WaitForDedicatedServerWorldActionIdleWithHooks(
        int actionPad,
        DWORD timeoutMs,
        const DedicatedServerWorldActionWaitHooks &hooks)
    {
        if (hooks.isIdleProc == nullptr)
        {
            return false;
        }

        const std::uint64_t start = LceGetMonotonicMilliseconds();
        while (!hooks.isIdleProc(actionPad, hooks.idleContext))
        {
            if (hooks.haltProc != nullptr &&
                hooks.haltProc(hooks.haltContext))
            {
                return false;
            }
            if (hooks.tickProc != nullptr)
            {
                hooks.tickProc(hooks.tickContext);
            }
            if (hooks.handleActionsProc != nullptr)
            {
                hooks.handleActionsProc(hooks.handleActionsContext);
            }
            if ((LceGetMonotonicMilliseconds() - start) >= timeoutMs)
            {
                return false;
            }
            LceSleepMilliseconds(10);
        }

        return true;
    }

    DedicatedServerPlatformRuntimeStartResult
    StartDedicatedServerPlatformRuntime(
        const DedicatedServerPlatformState &platformState);

    DedicatedServerAppSessionApplyResult ApplyDedicatedServerAppSessionPlan(
        const DedicatedServerAppSessionPlan &appSessionPlan);

    void TickDedicatedServerPlatformRuntime();
    void HandleDedicatedServerPlatformActions();
    bool IsDedicatedServerWorldActionIdle(int actionPad);
    void RequestDedicatedServerWorldAutosave(int actionPad);
    bool WaitForDedicatedServerWorldActionIdle(
        int actionPad,
        DWORD timeoutMs);
    bool HasDedicatedServerGameplayInstance();
    bool IsDedicatedServerAppShutdownRequested();
    void SetDedicatedServerAppShutdownRequested(bool shutdownRequested);
    bool IsDedicatedServerGameplayHalted();
    bool IsDedicatedServerStopSignalValid();
    void EnableDedicatedServerSaveOnExit();
    std::uint64_t GetDedicatedServerPlatformTickCount();
    std::uint64_t GetDedicatedServerAutosaveRequestCount();
    std::uint64_t GetDedicatedServerAutosaveCompletionCount();
    void HaltDedicatedServerGameplay();
    void WaitForDedicatedServerStopSignal();
    void StopDedicatedServerPlatformRuntime();
}
