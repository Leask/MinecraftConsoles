#pragma once

#include <cstdint>

namespace ServerRuntime
{
    enum ENativeDedicatedServerHostedGameWorkerCommandKind
    {
        eNativeDedicatedServerHostedGameWorkerCommand_None = 0,
        eNativeDedicatedServerHostedGameWorkerCommand_Save = 1,
        eNativeDedicatedServerHostedGameWorkerCommand_Stop = 2
    };

    struct NativeDedicatedServerHostedGameWorkerSnapshot
    {
        std::uint64_t pendingWorldActionTicks = 0;
        std::uint64_t pendingSaveCommands = 0;
        std::uint64_t pendingStopCommands = 0;
        std::uint64_t workerTickCount = 0;
        std::uint64_t completedWorldActions = 0;
        std::uint64_t processedSaveCommands = 0;
        std::uint64_t processedStopCommands = 0;
        std::uint64_t lastQueuedCommandId = 0;
        std::uint64_t lastProcessedCommandId = 0;
        ENativeDedicatedServerHostedGameWorkerCommandKind
            lastProcessedCommandKind =
                eNativeDedicatedServerHostedGameWorkerCommand_None;
    };

    void ResetNativeDedicatedServerHostedGameWorkerState();

    void ClearNativeDedicatedServerHostedGameWorkerState();

    void RequestNativeDedicatedServerHostedGameWorkerAutosave(
        unsigned int workTicks);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerSaveCommand();

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerStopCommand();

    void TickNativeDedicatedServerHostedGameWorker();

    bool IsNativeDedicatedServerHostedGameWorkerIdle();

    const char *GetNativeDedicatedServerHostedGameWorkerCommandKindName(
        ENativeDedicatedServerHostedGameWorkerCommandKind kind);

    NativeDedicatedServerHostedGameWorkerSnapshot
    GetNativeDedicatedServerHostedGameWorkerSnapshot();
}
