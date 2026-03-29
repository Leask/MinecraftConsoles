#pragma once

#include <cstdint>

namespace ServerRuntime
{
    struct DedicatedServerAutosaveTrackerSnapshot
    {
        bool autosavePending = false;
        bool lastObservedIdle = true;
        std::uint64_t requestCount = 0;
        std::uint64_t completionCount = 0;
    };

    void ResetDedicatedServerAutosaveTracker();

    void MarkDedicatedServerAutosaveTrackerRequested();

    void UpdateDedicatedServerAutosaveTracker(bool worldActionIdle);

    std::uint64_t GetDedicatedServerAutosaveTrackerRequestCount();

    std::uint64_t GetDedicatedServerAutosaveTrackerCompletionCount();

    DedicatedServerAutosaveTrackerSnapshot
    GetDedicatedServerAutosaveTrackerSnapshot();
}
