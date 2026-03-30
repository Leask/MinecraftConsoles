#pragma once

#include <cstdint>

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameWorkerSnapshot
    {
        std::uint64_t pendingWorldActionTicks = 0;
        std::uint64_t pendingSaveCommands = 0;
        std::uint64_t pendingStopCommands = 0;
        std::uint64_t workerTickCount = 0;
        std::uint64_t completedWorldActions = 0;
        std::uint64_t processedSaveCommands = 0;
        std::uint64_t processedStopCommands = 0;
    };

    void ResetNativeDedicatedServerHostedGameWorkerState();

    void ClearNativeDedicatedServerHostedGameWorkerState();

    void RequestNativeDedicatedServerHostedGameWorkerAutosave(
        unsigned int workTicks);

    void EnqueueNativeDedicatedServerHostedGameWorkerSaveCommand();

    void EnqueueNativeDedicatedServerHostedGameWorkerStopCommand();

    void TickNativeDedicatedServerHostedGameWorker();

    bool IsNativeDedicatedServerHostedGameWorkerIdle();

    NativeDedicatedServerHostedGameWorkerSnapshot
    GetNativeDedicatedServerHostedGameWorkerSnapshot();
}
