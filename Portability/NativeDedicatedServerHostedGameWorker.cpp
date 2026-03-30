#include "NativeDedicatedServerHostedGameWorker.h"

#include <atomic>
#include <cstdint>

namespace
{
    std::atomic<unsigned int> g_nativeHostedWorkerPendingWorldActionTicks(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerTickCount(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerCompletedActions(0);
}

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameWorkerState()
    {
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
        g_nativeHostedWorkerTickCount.store(0);
        g_nativeHostedWorkerCompletedActions.store(0);
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
                g_nativeHostedWorkerTickCount.fetch_add(1);
                if (current == 1)
                {
                    g_nativeHostedWorkerCompletedActions.fetch_add(1);
                }
                break;
            }
        }
    }

    bool IsNativeDedicatedServerHostedGameWorkerIdle()
    {
        return g_nativeHostedWorkerPendingWorldActionTicks.load() == 0;
    }

    NativeDedicatedServerHostedGameWorkerSnapshot
    GetNativeDedicatedServerHostedGameWorkerSnapshot()
    {
        NativeDedicatedServerHostedGameWorkerSnapshot snapshot = {};
        snapshot.pendingWorldActionTicks =
            g_nativeHostedWorkerPendingWorldActionTicks.load();
        snapshot.workerTickCount =
            g_nativeHostedWorkerTickCount.load();
        snapshot.completedWorldActions =
            g_nativeHostedWorkerCompletedActions.load();
        return snapshot;
    }
}
