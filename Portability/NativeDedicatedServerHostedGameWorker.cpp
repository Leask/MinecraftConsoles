#include "NativeDedicatedServerHostedGameWorker.h"

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"

namespace
{
    struct NativeDedicatedServerHostedGameWorkerCommand
    {
        std::uint64_t id = 0;
        ServerRuntime::ENativeDedicatedServerHostedGameWorkerCommandKind
            kind =
                ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_None;
    };

    std::mutex g_nativeHostedWorkerQueueMutex;
    std::deque<NativeDedicatedServerHostedGameWorkerCommand>
        g_nativeHostedWorkerCommandQueue;
    std::atomic<unsigned int> g_nativeHostedWorkerPendingWorldActionTicks(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingSaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingStopCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerTickCount(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerCompletedActions(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedSaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedStopCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerNextCommandId(1);
    std::atomic<std::uint64_t> g_nativeHostedWorkerLastQueuedCommandId(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerLastProcessedCommandId(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerLastProcessedCommandKind(0);
}

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameWorkerState()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
        g_nativeHostedWorkerCommandQueue.clear();
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
        g_nativeHostedWorkerPendingSaveCommands.store(0);
        g_nativeHostedWorkerPendingStopCommands.store(0);
        g_nativeHostedWorkerTickCount.store(0);
        g_nativeHostedWorkerCompletedActions.store(0);
        g_nativeHostedWorkerProcessedSaveCommands.store(0);
        g_nativeHostedWorkerProcessedStopCommands.store(0);
        g_nativeHostedWorkerNextCommandId.store(1);
        g_nativeHostedWorkerLastQueuedCommandId.store(0);
        g_nativeHostedWorkerLastProcessedCommandId.store(0);
        g_nativeHostedWorkerLastProcessedCommandKind.store(0);
    }

    void ClearNativeDedicatedServerHostedGameWorkerState()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
        g_nativeHostedWorkerCommandQueue.clear();
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

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerSaveCommand()
    {
        const std::uint64_t commandId =
            g_nativeHostedWorkerNextCommandId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
            g_nativeHostedWorkerCommandQueue.push_back(
                NativeDedicatedServerHostedGameWorkerCommand{
                    commandId,
                    eNativeDedicatedServerHostedGameWorkerCommand_Save});
        }
        g_nativeHostedWorkerPendingSaveCommands.fetch_add(1);
        g_nativeHostedWorkerLastQueuedCommandId.store(commandId);
        return commandId;
    }

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerStopCommand()
    {
        const std::uint64_t commandId =
            g_nativeHostedWorkerNextCommandId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
            g_nativeHostedWorkerCommandQueue.push_back(
                NativeDedicatedServerHostedGameWorkerCommand{
                    commandId,
                    eNativeDedicatedServerHostedGameWorkerCommand_Stop});
        }
        g_nativeHostedWorkerPendingStopCommands.fetch_add(1);
        g_nativeHostedWorkerLastQueuedCommandId.store(commandId);
        return commandId;
    }

    void TickNativeDedicatedServerHostedGameWorker()
    {
        NativeDedicatedServerHostedGameWorkerCommand command = {};
        {
            std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
            if (!g_nativeHostedWorkerCommandQueue.empty())
            {
                command = g_nativeHostedWorkerCommandQueue.front();
                g_nativeHostedWorkerCommandQueue.pop_front();
            }
        }

        if (command.kind == eNativeDedicatedServerHostedGameWorkerCommand_Save)
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
                    g_nativeHostedWorkerLastProcessedCommandId.store(command.id);
                    g_nativeHostedWorkerLastProcessedCommandKind.store(
                        eNativeDedicatedServerHostedGameWorkerCommand_Save);
                    break;
                }
            }
        }
        else if (
            command.kind == eNativeDedicatedServerHostedGameWorkerCommand_Stop)
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
                    g_nativeHostedWorkerLastProcessedCommandId.store(command.id);
                    g_nativeHostedWorkerLastProcessedCommandKind.store(
                        eNativeDedicatedServerHostedGameWorkerCommand_Stop);
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

    const char *GetNativeDedicatedServerHostedGameWorkerCommandKindName(
        ENativeDedicatedServerHostedGameWorkerCommandKind kind)
    {
        switch (kind)
        {
        case eNativeDedicatedServerHostedGameWorkerCommand_Save:
            return "save";
        case eNativeDedicatedServerHostedGameWorkerCommand_Stop:
            return "stop";
        default:
            return "none";
        }
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
        snapshot.lastQueuedCommandId =
            g_nativeHostedWorkerLastQueuedCommandId.load();
        snapshot.lastProcessedCommandId =
            g_nativeHostedWorkerLastProcessedCommandId.load();
        snapshot.lastProcessedCommandKind =
            (ENativeDedicatedServerHostedGameWorkerCommandKind)
                g_nativeHostedWorkerLastProcessedCommandKind.load();
        return snapshot;
    }
}
