#pragma once

#include <cstddef>

#include "DedicatedServerRuntime.h"

namespace ServerRuntime
{
    typedef void DedicatedServerGameplayLoopPollProc(void* pollContext);

    struct DedicatedServerGameplayLoopIterationResult
    {
        bool shouldExit = false;
        bool autosaveCompleted = false;
        bool autosaveRequested = false;
        bool advancedDeadline = false;
    };

    struct DedicatedServerGameplayLoopRunResult
    {
        std::size_t iterations = 0;
        bool requestedAppShutdown = false;
        DedicatedServerGameplayLoopIterationResult lastIteration = {};
    };

    DedicatedServerGameplayLoopIterationResult
    TickDedicatedServerGameplayLoop(
        DedicatedServerAutosaveState *autosaveState,
        const ServerPropertiesConfig &serverProperties,
        int actionPad,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext);

    DedicatedServerGameplayLoopRunResult
    RunDedicatedServerGameplayLoopUntilExit(
        DedicatedServerAutosaveState *autosaveState,
        const ServerPropertiesConfig &serverProperties,
        int actionPad,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext,
        unsigned int sleepMs);
}
