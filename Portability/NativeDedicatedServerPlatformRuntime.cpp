#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"

namespace ServerRuntime
{
    namespace
    {
        bool g_nativeGameplayInstance = false;
        bool g_nativeAppShutdownRequested = false;
        bool g_nativeGameplayHalted = false;
        bool g_nativeStopSignalValid = false;
    }

    DedicatedServerPlatformRuntimeStartResult
    StartDedicatedServerPlatformRuntime(
        const DedicatedServerPlatformState &)
    {
        DedicatedServerPlatformRuntimeStartResult result = {};
        g_nativeGameplayInstance = true;
        g_nativeAppShutdownRequested = false;
        g_nativeGameplayHalted = false;
        g_nativeStopSignalValid = true;
        result.ok = true;
        result.exitCode = 0;
        result.runtimeName = "NativeDesktopStub";
        result.headless = true;
        return result;
    }

    void TickDedicatedServerPlatformRuntime()
    {
    }

    void HandleDedicatedServerPlatformActions()
    {
    }

    bool IsDedicatedServerWorldActionIdle(int)
    {
        return true;
    }

    void RequestDedicatedServerWorldAutosave(int)
    {
    }

    bool WaitForDedicatedServerWorldActionIdle(
        int,
        DWORD)
    {
        return true;
    }

    bool HasDedicatedServerGameplayInstance()
    {
        return g_nativeGameplayInstance;
    }

    bool IsDedicatedServerAppShutdownRequested()
    {
        return g_nativeAppShutdownRequested;
    }

    void SetDedicatedServerAppShutdownRequested(bool shutdownRequested)
    {
        g_nativeAppShutdownRequested = shutdownRequested;
    }

    bool IsDedicatedServerGameplayHalted()
    {
        return g_nativeGameplayHalted;
    }

    bool IsDedicatedServerStopSignalValid()
    {
        return g_nativeStopSignalValid;
    }

    void EnableDedicatedServerSaveOnExit()
    {
    }

    void HaltDedicatedServerGameplay()
    {
        g_nativeGameplayHalted = true;
    }

    void WaitForDedicatedServerStopSignal()
    {
    }

    void StopDedicatedServerPlatformRuntime()
    {
        g_nativeGameplayInstance = false;
        g_nativeStopSignalValid = false;
    }
}
