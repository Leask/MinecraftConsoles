#include "DedicatedServerAutosaveTracker.h"

namespace
{
    ServerRuntime::DedicatedServerAutosaveTrackerSnapshot
        g_dedicatedServerAutosaveTracker = {};
}

namespace ServerRuntime
{
    void ResetDedicatedServerAutosaveTracker()
    {
        g_dedicatedServerAutosaveTracker =
            DedicatedServerAutosaveTrackerSnapshot{};
    }

    void MarkDedicatedServerAutosaveTrackerRequested()
    {
        ++g_dedicatedServerAutosaveTracker.requestCount;
        g_dedicatedServerAutosaveTracker.autosavePending = true;
        g_dedicatedServerAutosaveTracker.lastObservedIdle = false;
    }

    void UpdateDedicatedServerAutosaveTracker(bool worldActionIdle)
    {
        if (g_dedicatedServerAutosaveTracker.autosavePending &&
            worldActionIdle &&
            !g_dedicatedServerAutosaveTracker.lastObservedIdle)
        {
            ++g_dedicatedServerAutosaveTracker.completionCount;
            g_dedicatedServerAutosaveTracker.autosavePending = false;
        }

        g_dedicatedServerAutosaveTracker.lastObservedIdle = worldActionIdle;
    }

    std::uint64_t GetDedicatedServerAutosaveTrackerRequestCount()
    {
        return g_dedicatedServerAutosaveTracker.requestCount;
    }

    std::uint64_t GetDedicatedServerAutosaveTrackerCompletionCount()
    {
        return g_dedicatedServerAutosaveTracker.completionCount;
    }

    DedicatedServerAutosaveTrackerSnapshot
    GetDedicatedServerAutosaveTrackerSnapshot()
    {
        return g_dedicatedServerAutosaveTracker;
    }
}
