#pragma once

#include <cstdint>

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameWorkerSnapshot
    {
        std::uint64_t pendingWorldActionTicks = 0;
        std::uint64_t workerTickCount = 0;
        std::uint64_t completedWorldActions = 0;
    };

    void ResetNativeDedicatedServerHostedGameWorkerState();

    void ClearNativeDedicatedServerHostedGameWorkerState();

    void RequestNativeDedicatedServerHostedGameWorkerAutosave(
        unsigned int workTicks);

    void TickNativeDedicatedServerHostedGameWorker();

    bool IsNativeDedicatedServerHostedGameWorkerIdle();

    NativeDedicatedServerHostedGameWorkerSnapshot
    GetNativeDedicatedServerHostedGameWorkerSnapshot();
}
