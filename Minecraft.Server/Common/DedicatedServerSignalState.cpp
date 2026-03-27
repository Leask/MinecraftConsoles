#include "DedicatedServerSignalState.h"

#include <atomic>

namespace
{
    std::atomic<bool> g_dedicatedServerShutdownRequested(false);
}

namespace ServerRuntime
{
    bool IsDedicatedServerShutdownRequested()
    {
        return g_dedicatedServerShutdownRequested.load();
    }

    void RequestDedicatedServerShutdown()
    {
        g_dedicatedServerShutdownRequested.store(true);
    }

    void ResetDedicatedServerShutdownRequest()
    {
        g_dedicatedServerShutdownRequested.store(false);
    }
}
