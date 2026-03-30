#include "NativeDedicatedServerHostedGameWorker.h"

#include <atomic>
#include <cstdint>

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"

namespace
{
    std::atomic<unsigned int> g_nativeHostedWorkerPendingWorldActionTicks(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingSaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingStopCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerTickCount(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerCompletedActions(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedSaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedStopCommands(0);
}

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameWorkerState()
    {
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
        g_nativeHostedWorkerPendingSaveCommands.store(0);
        g_nativeHostedWorkerPendingStopCommands.store(0);
        g_nativeHostedWorkerTickCount.store(0);
        g_nativeHostedWorkerCompletedActions.store(0);
        g_nativeHostedWorkerProcessedSaveCommands.store(0);
        g_nativeHostedWorkerProcessedStopCommands.store(0);
    }

    void ClearNativeDedicatedServerHostedGameWorkerState()
    {
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
        g_nativeHostedWorkerPendingSaveCommands.store(0);
        g_nativeHostedWorkerPendingStopCommands.store(0);
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

    void EnqueueNativeDedicatedServerHostedGameWorkerSaveCommand()
    {
        g_nativeHostedWorkerPendingSaveCommands.fetch_add(1);
    }

    void EnqueueNativeDedicatedServerHostedGameWorkerStopCommand()
    {
        g_nativeHostedWorkerPendingStopCommands.fetch_add(1);
    }

    void TickNativeDedicatedServerHostedGameWorker()
    {
        std::uint64_t pendingSaveCommands =
            g_nativeHostedWorkerPendingSaveCommands.load();
        while (pendingSaveCommands > 0)
        {
            if (g_nativeHostedWorkerPendingSaveCommands
                    .compare_exchange_weak(
                        pendingSaveCommands,
                        pendingSaveCommands - 1))
            {
                RequestDedicatedServerWorldAutosave(0);
                g_nativeHostedWorkerProcessedSaveCommands.fetch_add(1);
                break;
            }
        }

        if (pendingSaveCommands == 0)
        {
            std::uint64_t pendingStopCommands =
                g_nativeHostedWorkerPendingStopCommands.load();
            while (pendingStopCommands > 0)
            {
                if (g_nativeHostedWorkerPendingStopCommands
                        .compare_exchange_weak(
                            pendingStopCommands,
                            pendingStopCommands - 1))
                {
                    RequestDedicatedServerShutdown();
                    g_nativeHostedWorkerProcessedStopCommands.fetch_add(1);
                    break;
                }
            }
        }

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
        return g_nativeHostedWorkerPendingWorldActionTicks.load() == 0 &&
            g_nativeHostedWorkerPendingSaveCommands.load() == 0 &&
            g_nativeHostedWorkerPendingStopCommands.load() == 0;
    }

    NativeDedicatedServerHostedGameWorkerSnapshot
    GetNativeDedicatedServerHostedGameWorkerSnapshot()
    {
        NativeDedicatedServerHostedGameWorkerSnapshot snapshot = {};
        snapshot.pendingWorldActionTicks =
            g_nativeHostedWorkerPendingWorldActionTicks.load();
        snapshot.pendingSaveCommands =
            g_nativeHostedWorkerPendingSaveCommands.load();
        snapshot.pendingStopCommands =
            g_nativeHostedWorkerPendingStopCommands.load();
        snapshot.workerTickCount =
            g_nativeHostedWorkerTickCount.load();
        snapshot.completedWorldActions =
            g_nativeHostedWorkerCompletedActions.load();
        snapshot.processedSaveCommands =
            g_nativeHostedWorkerProcessedSaveCommands.load();
        snapshot.processedStopCommands =
            g_nativeHostedWorkerProcessedStopCommands.load();
        return snapshot;
    }
}
