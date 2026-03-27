#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"

namespace ServerRuntime
{
    DedicatedServerPlatformRuntimeStartResult
    StartDedicatedServerPlatformRuntime(
        const DedicatedServerPlatformState &)
    {
        DedicatedServerPlatformRuntimeStartResult result = {};
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

    void StopDedicatedServerPlatformRuntime()
    {
    }
}
