#pragma once

#include <cstdint>

namespace ServerRuntime
{
    enum ENativeDedicatedServerHostedGameWorkerCommandKind
    {
        eNativeDedicatedServerHostedGameWorkerCommand_None = 0,
        eNativeDedicatedServerHostedGameWorkerCommand_Autosave = 1,
        eNativeDedicatedServerHostedGameWorkerCommand_Save = 2,
        eNativeDedicatedServerHostedGameWorkerCommand_Stop = 3,
        eNativeDedicatedServerHostedGameWorkerCommand_Halt = 4
    };

    struct NativeDedicatedServerHostedGameWorkerSnapshot
    {
        std::uint64_t pendingWorldActionTicks = 0;
        std::uint64_t pendingAutosaveCommands = 0;
        std::uint64_t pendingSaveCommands = 0;
        std::uint64_t pendingStopCommands = 0;
        std::uint64_t pendingHaltCommands = 0;
        std::uint64_t workerTickCount = 0;
        std::uint64_t completedWorldActions = 0;
        std::uint64_t processedAutosaveCommands = 0;
        std::uint64_t processedSaveCommands = 0;
        std::uint64_t processedStopCommands = 0;
        std::uint64_t processedHaltCommands = 0;
        std::uint64_t lastQueuedCommandId = 0;
        std::uint64_t activeCommandId = 0;
        std::uint64_t activeCommandTicksRemaining = 0;
        ENativeDedicatedServerHostedGameWorkerCommandKind
            activeCommandKind =
                eNativeDedicatedServerHostedGameWorkerCommand_None;
        std::uint64_t lastProcessedCommandId = 0;
        ENativeDedicatedServerHostedGameWorkerCommandKind
            lastProcessedCommandKind =
                eNativeDedicatedServerHostedGameWorkerCommand_None;
    };

}
