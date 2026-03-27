#pragma once

namespace ServerRuntime
{
    bool IsDedicatedServerShutdownRequested();

    void RequestDedicatedServerShutdown();

    void ResetDedicatedServerShutdownRequest();
}
