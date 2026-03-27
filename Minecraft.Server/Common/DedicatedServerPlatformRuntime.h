#pragma once

#include <string>

#include <lce_win32/lce_win32.h>

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
    void HaltDedicatedServerGameplay();
    void WaitForDedicatedServerStopSignal();
    void StopDedicatedServerPlatformRuntime();
}
