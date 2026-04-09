#include "NativeDedicatedServerHostedGameWorker.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>

#include "Minecraft.Server/Common/DedicatedServerAutosaveTracker.h"
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
        unsigned int workTicks = 0;
    };

    std::mutex g_nativeHostedWorkerQueueMutex;
    std::deque<NativeDedicatedServerHostedGameWorkerCommand>
        g_nativeHostedWorkerCommandQueue;
    std::atomic<unsigned int> g_nativeHostedWorkerPendingWorldActionTicks(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingAutosaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingSaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingStopCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerPendingHaltCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerTickCount(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerCompletedActions(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedAutosaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedSaveCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedStopCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerProcessedHaltCommands(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerNextCommandId(1);
    std::atomic<std::uint64_t> g_nativeHostedWorkerLastQueuedCommandId(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerActiveCommandId(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerActiveCommandTicks(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerActiveCommandKind(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerLastProcessedCommandId(0);
    std::atomic<std::uint64_t> g_nativeHostedWorkerLastProcessedCommandKind(0);
}

namespace ServerRuntime
{
    namespace
    {
        constexpr std::uint64_t kNativeHostedGameWorkerFrameDelayMs = 10;

        bool IsNativeDedicatedServerHostedGameWorkerShutdownRequested()
        {
            return ServerRuntime::IsDedicatedServerShutdownRequested() ||
                ServerRuntime::IsDedicatedServerAppShutdownRequested() ||
                ServerRuntime::IsDedicatedServerGameplayHalted();
        }

        bool TryConsumePendingNativeDedicatedServerHostedGameWorkerCommand(
            std::atomic<std::uint64_t> *pendingCounter)
        {
            if (pendingCounter == nullptr)
            {
                return false;
            }

            std::uint64_t pending = pendingCounter->load();
            while (pending > 0)
            {
                if (pendingCounter->compare_exchange_weak(
                        pending,
                        pending - 1))
                {
                    return true;
                }
            }

            return false;
        }

        void SetNativeDedicatedServerHostedGameWorkerActiveCommand(
            const NativeDedicatedServerHostedGameWorkerCommand &command,
            std::uint64_t remainingTicks)
        {
            g_nativeHostedWorkerActiveCommandId.store(command.id);
            g_nativeHostedWorkerActiveCommandKind.store(command.kind);
            g_nativeHostedWorkerActiveCommandTicks.store(remainingTicks);
        }

        void ClearNativeDedicatedServerHostedGameWorkerActiveCommand()
        {
            g_nativeHostedWorkerActiveCommandId.store(0);
            g_nativeHostedWorkerActiveCommandKind.store(
                eNativeDedicatedServerHostedGameWorkerCommand_None);
            g_nativeHostedWorkerActiveCommandTicks.store(0);
        }

        void ApplyNativeDedicatedServerHostedGameWorkerWorldActionTicks(
            unsigned int workTicks)
        {
            unsigned int current =
                g_nativeHostedWorkerPendingWorldActionTicks.load();
            while (current < workTicks &&
                !g_nativeHostedWorkerPendingWorldActionTicks
                    .compare_exchange_weak(
                        current,
                        workTicks))
            {
            }
        }

        void CompleteNativeDedicatedServerHostedGameWorkerCommand(
            const NativeDedicatedServerHostedGameWorkerCommand &command)
        {
            if (command.kind ==
                eNativeDedicatedServerHostedGameWorkerCommand_Autosave)
            {
                g_nativeHostedWorkerCompletedActions.fetch_add(1);
                g_nativeHostedWorkerProcessedAutosaveCommands.fetch_add(1);
                g_nativeHostedWorkerPendingWorldActionTicks.store(0);
            }
            else if (
                command.kind == eNativeDedicatedServerHostedGameWorkerCommand_Save)
            {
                g_nativeHostedWorkerProcessedSaveCommands.fetch_add(1);
            }
            else if (
                command.kind == eNativeDedicatedServerHostedGameWorkerCommand_Stop)
            {
                g_nativeHostedWorkerProcessedStopCommands.fetch_add(1);
            }
            else if (
                command.kind == eNativeDedicatedServerHostedGameWorkerCommand_Halt)
            {
                g_nativeHostedWorkerProcessedHaltCommands.fetch_add(1);
            }

            g_nativeHostedWorkerLastProcessedCommandId.store(command.id);
            g_nativeHostedWorkerLastProcessedCommandKind.store(command.kind);
            ClearNativeDedicatedServerHostedGameWorkerActiveCommand();
        }

        bool BeginNativeDedicatedServerHostedGameWorkerCommand(
            const NativeDedicatedServerHostedGameWorkerCommand &command)
        {
            if (command.kind ==
                eNativeDedicatedServerHostedGameWorkerCommand_Autosave)
            {
                if (!TryConsumePendingNativeDedicatedServerHostedGameWorkerCommand(
                        &g_nativeHostedWorkerPendingAutosaveCommands))
                {
                    return false;
                }

                const std::uint64_t workTicks = std::max<std::uint64_t>(
                    command.workTicks,
                    1);
                ApplyNativeDedicatedServerHostedGameWorkerWorldActionTicks(
                    (unsigned int)workTicks);
                SetNativeDedicatedServerHostedGameWorkerActiveCommand(
                    command,
                    workTicks);
                return true;
            }

            if (command.kind ==
                eNativeDedicatedServerHostedGameWorkerCommand_Save)
            {
                if (!TryConsumePendingNativeDedicatedServerHostedGameWorkerCommand(
                        &g_nativeHostedWorkerPendingSaveCommands))
                {
                    return false;
                }

                RequestNativeDedicatedServerHostedGameWorkerAutosave(2);
                SetNativeDedicatedServerHostedGameWorkerActiveCommand(
                    command,
                    1);
                return true;
            }

            if (command.kind ==
                eNativeDedicatedServerHostedGameWorkerCommand_Stop)
            {
                if (!TryConsumePendingNativeDedicatedServerHostedGameWorkerCommand(
                        &g_nativeHostedWorkerPendingStopCommands))
                {
                    return false;
                }

                RequestDedicatedServerShutdown();
                SetNativeDedicatedServerHostedGameWorkerActiveCommand(
                    command,
                    1);
                return true;
            }

            if (command.kind ==
                eNativeDedicatedServerHostedGameWorkerCommand_Halt)
            {
                if (!TryConsumePendingNativeDedicatedServerHostedGameWorkerCommand(
                        &g_nativeHostedWorkerPendingHaltCommands))
                {
                    return false;
                }

                RequestDedicatedServerShutdown();
                SetNativeDedicatedServerHostedGameWorkerActiveCommand(
                    command,
                    1);
                return true;
            }

            return false;
        }
    }

    void ResetNativeDedicatedServerHostedGameWorkerState()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
        g_nativeHostedWorkerCommandQueue.clear();
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
        g_nativeHostedWorkerPendingAutosaveCommands.store(0);
        g_nativeHostedWorkerPendingSaveCommands.store(0);
        g_nativeHostedWorkerPendingStopCommands.store(0);
        g_nativeHostedWorkerPendingHaltCommands.store(0);
        g_nativeHostedWorkerTickCount.store(0);
        g_nativeHostedWorkerCompletedActions.store(0);
        g_nativeHostedWorkerProcessedAutosaveCommands.store(0);
        g_nativeHostedWorkerProcessedSaveCommands.store(0);
        g_nativeHostedWorkerProcessedStopCommands.store(0);
        g_nativeHostedWorkerProcessedHaltCommands.store(0);
        g_nativeHostedWorkerNextCommandId.store(1);
        g_nativeHostedWorkerLastQueuedCommandId.store(0);
        ClearNativeDedicatedServerHostedGameWorkerActiveCommand();
        g_nativeHostedWorkerLastProcessedCommandId.store(0);
        g_nativeHostedWorkerLastProcessedCommandKind.store(0);
    }

    void ClearNativeDedicatedServerHostedGameWorkerState()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
        g_nativeHostedWorkerCommandQueue.clear();
        g_nativeHostedWorkerPendingWorldActionTicks.store(0);
        g_nativeHostedWorkerPendingAutosaveCommands.store(0);
        g_nativeHostedWorkerPendingSaveCommands.store(0);
        g_nativeHostedWorkerPendingStopCommands.store(0);
        g_nativeHostedWorkerPendingHaltCommands.store(0);
        ClearNativeDedicatedServerHostedGameWorkerActiveCommand();
    }

    void RequestNativeDedicatedServerHostedGameWorkerAutosave(
        unsigned int workTicks)
    {
        MarkDedicatedServerAutosaveTrackerRequested();
        const std::uint64_t commandId =
            g_nativeHostedWorkerNextCommandId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
            g_nativeHostedWorkerCommandQueue.push_back(
                NativeDedicatedServerHostedGameWorkerCommand{
                    commandId,
                    eNativeDedicatedServerHostedGameWorkerCommand_Autosave,
                    workTicks});
        }
        g_nativeHostedWorkerPendingAutosaveCommands.fetch_add(1);
        g_nativeHostedWorkerLastQueuedCommandId.store(commandId);
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

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerHaltCommand()
    {
        const std::uint64_t commandId =
            g_nativeHostedWorkerNextCommandId.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
            g_nativeHostedWorkerCommandQueue.push_back(
                NativeDedicatedServerHostedGameWorkerCommand{
                    commandId,
                    eNativeDedicatedServerHostedGameWorkerCommand_Halt});
        }
        g_nativeHostedWorkerPendingHaltCommands.fetch_add(1);
        g_nativeHostedWorkerLastQueuedCommandId.store(commandId);
        return commandId;
    }

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerHaltSequence(
        bool requestAutosaveFirst,
        unsigned int autosaveWorkTicks)
    {
        if (requestAutosaveFirst)
        {
            RequestNativeDedicatedServerHostedGameWorkerAutosave(
                autosaveWorkTicks);
        }

        return EnqueueNativeDedicatedServerHostedGameWorkerHaltCommand();
    }

    void TickNativeDedicatedServerHostedGameWorker()
    {
        NativeDedicatedServerHostedGameWorkerCommand command = {};
        if ((ENativeDedicatedServerHostedGameWorkerCommandKind)
                g_nativeHostedWorkerActiveCommandKind.load() ==
            eNativeDedicatedServerHostedGameWorkerCommand_None)
        {
            {
                std::lock_guard<std::mutex> lock(g_nativeHostedWorkerQueueMutex);
                if (!g_nativeHostedWorkerCommandQueue.empty())
                {
                    command = g_nativeHostedWorkerCommandQueue.front();
                    g_nativeHostedWorkerCommandQueue.pop_front();
                }
            }

            if (command.kind !=
                eNativeDedicatedServerHostedGameWorkerCommand_None)
            {
                BeginNativeDedicatedServerHostedGameWorkerCommand(command);
            }
        }

        command.id = g_nativeHostedWorkerActiveCommandId.load();
        command.kind =
            (ENativeDedicatedServerHostedGameWorkerCommandKind)
                g_nativeHostedWorkerActiveCommandKind.load();
        std::uint64_t remaining =
            g_nativeHostedWorkerActiveCommandTicks.load();
        while (command.kind !=
                eNativeDedicatedServerHostedGameWorkerCommand_None &&
            remaining > 0)
        {
            if (g_nativeHostedWorkerActiveCommandTicks
                    .compare_exchange_weak(
                        remaining,
                        remaining - 1))
            {
                g_nativeHostedWorkerTickCount.fetch_add(1);
                if (command.kind ==
                    eNativeDedicatedServerHostedGameWorkerCommand_Autosave)
                {
                    g_nativeHostedWorkerPendingWorldActionTicks.store(
                        remaining - 1);
                }

                if (remaining == 1)
                {
                    CompleteNativeDedicatedServerHostedGameWorkerCommand(
                        command);
                }
                break;
            }
        }
    }

    NativeDedicatedServerHostedGameWorkerFrameResult
    TickNativeDedicatedServerHostedGameWorkerFrame()
    {
        TickNativeDedicatedServerHostedGameWorker();
        NativeDedicatedServerHostedGameWorkerFrameResult result = {};
        result.snapshot = GetNativeDedicatedServerHostedGameWorkerSnapshot();
        const bool idle = IsNativeDedicatedServerHostedGameWorkerIdle();
        result.shouldStopRunning =
            IsNativeDedicatedServerHostedGameWorkerShutdownRequested() &&
            idle;
        result.nextSleepDurationMs =
            result.shouldStopRunning ? 0U : kNativeHostedGameWorkerFrameDelayMs;
        UpdateDedicatedServerAutosaveTracker(idle);
        return result;
    }

    bool IsNativeDedicatedServerHostedGameWorkerIdle()
    {
        return g_nativeHostedWorkerPendingWorldActionTicks.load() == 0 &&
            g_nativeHostedWorkerPendingAutosaveCommands.load() == 0 &&
            g_nativeHostedWorkerPendingSaveCommands.load() == 0 &&
            g_nativeHostedWorkerPendingStopCommands.load() == 0 &&
            g_nativeHostedWorkerPendingHaltCommands.load() == 0 &&
            (ENativeDedicatedServerHostedGameWorkerCommandKind)
                    g_nativeHostedWorkerActiveCommandKind.load() ==
                eNativeDedicatedServerHostedGameWorkerCommand_None;
    }

    const char *GetNativeDedicatedServerHostedGameWorkerCommandKindName(
        ENativeDedicatedServerHostedGameWorkerCommandKind kind)
    {
        switch (kind)
        {
        case eNativeDedicatedServerHostedGameWorkerCommand_Autosave:
            return "autosave";
        case eNativeDedicatedServerHostedGameWorkerCommand_Save:
            return "save";
        case eNativeDedicatedServerHostedGameWorkerCommand_Stop:
            return "stop";
        case eNativeDedicatedServerHostedGameWorkerCommand_Halt:
            return "halt";
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
        snapshot.pendingAutosaveCommands =
            g_nativeHostedWorkerPendingAutosaveCommands.load();
        snapshot.pendingSaveCommands =
            g_nativeHostedWorkerPendingSaveCommands.load();
        snapshot.pendingStopCommands =
            g_nativeHostedWorkerPendingStopCommands.load();
        snapshot.pendingHaltCommands =
            g_nativeHostedWorkerPendingHaltCommands.load();
        snapshot.workerTickCount =
            g_nativeHostedWorkerTickCount.load();
        snapshot.completedWorldActions =
            g_nativeHostedWorkerCompletedActions.load();
        snapshot.processedAutosaveCommands =
            g_nativeHostedWorkerProcessedAutosaveCommands.load();
        snapshot.processedSaveCommands =
            g_nativeHostedWorkerProcessedSaveCommands.load();
        snapshot.processedStopCommands =
            g_nativeHostedWorkerProcessedStopCommands.load();
        snapshot.processedHaltCommands =
            g_nativeHostedWorkerProcessedHaltCommands.load();
        snapshot.lastQueuedCommandId =
            g_nativeHostedWorkerLastQueuedCommandId.load();
        snapshot.activeCommandId =
            g_nativeHostedWorkerActiveCommandId.load();
        snapshot.activeCommandTicksRemaining =
            g_nativeHostedWorkerActiveCommandTicks.load();
        snapshot.activeCommandKind =
            (ENativeDedicatedServerHostedGameWorkerCommandKind)
                g_nativeHostedWorkerActiveCommandKind.load();
        snapshot.lastProcessedCommandId =
            g_nativeHostedWorkerLastProcessedCommandId.load();
        snapshot.lastProcessedCommandKind =
            (ENativeDedicatedServerHostedGameWorkerCommandKind)
                g_nativeHostedWorkerLastProcessedCommandKind.load();
        return snapshot;
    }

    std::uint64_t GetNativeDedicatedServerHostedGameWorkerAutosaveCompletions()
    {
        return std::max(
            GetDedicatedServerAutosaveTrackerCompletionCount(),
            g_nativeHostedWorkerProcessedAutosaveCommands.load());
    }
}
