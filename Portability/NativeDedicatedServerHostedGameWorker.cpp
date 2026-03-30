#include "NativeDedicatedServerHostedGameWorker.h"

#include <atomic>

namespace
{
    std::atomic<unsigned int> g_nativeHostedWorkerPendingWorldActionTicks(0);
}

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameWorkerState()
    {
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
    }

    void ClearNativeDedicatedServerHostedGameWorkerState()
    {
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
    }

    void RequestNativeDedicatedServerHostedGameWorkerAutosave(
        unsigned int workTicks)
    {
        unsigned int current =
            g_nativeHostedWorkerPendingWorldActionTicks.load();
        while (current < workTicks &&
            !g_nativeHostedWorkerPendingWorldActionTicks.compare_exchange_weak(
                current,
                workTicks))
        {
        }
    }

    void TickNativeDedicatedServerHostedGameWorker()
    {
        unsigned int current =
            g_nativeHostedWorkerPendingWorldActionTicks.load();
        while (current > 0)
        {
            if (g_nativeHostedWorkerPendingWorldActionTicks
                    .compare_exchange_weak(
                        current,
                        current - 1))
            {
                break;
            }
        }
    }

    bool IsNativeDedicatedServerHostedGameWorkerIdle()
    {
        return g_nativeHostedWorkerPendingWorldActionTicks.load() == 0;
    }
}
