#include "DedicatedServerRuntime.h"

#include "StringUtils.h"

namespace
{
    static constexpr std::uint64_t kDefaultAutosaveIntervalMs = 60U * 1000U;
}

namespace ServerRuntime
{
    DedicatedServerRuntimeState BuildDedicatedServerRuntimeState(
        const DedicatedServerConfig &config,
        const ServerPropertiesConfig &serverProperties)
    {
        DedicatedServerRuntimeState state = {};
        state.hostNameUtf8 =
            config.name[0] != 0 ? config.name : "DedicatedServer";
        state.hostNameWide = StringUtils::Utf8ToWide(state.hostNameUtf8);
        if (state.hostNameWide.empty())
        {
            state.hostNameWide = L"DedicatedServer";
        }

        state.bindIp = config.bindIP[0] != 0 ? config.bindIP : "0.0.0.0";
        state.multiplayerPort = config.port;
        state.dedicatedServerPort = config.port;
        state.multiplayerHost = true;
        state.multiplayerJoin = false;
        state.dedicatedServer = true;
        state.lanAdvertise = serverProperties.lanAdvertise;
        return state;
    }

    std::uint64_t GetDedicatedServerAutosaveIntervalMs(
        const ServerPropertiesConfig &serverProperties)
    {
        if (serverProperties.autosaveIntervalSeconds <= 0)
        {
            return kDefaultAutosaveIntervalMs;
        }

        return
            static_cast<std::uint64_t>(
                serverProperties.autosaveIntervalSeconds) * 1000U;
    }

    std::uint64_t ComputeNextDedicatedServerAutosaveDeadlineMs(
        std::uint64_t nowMs,
        const ServerPropertiesConfig &serverProperties)
    {
        return nowMs + GetDedicatedServerAutosaveIntervalMs(serverProperties);
    }

    DedicatedServerAutosaveState CreateDedicatedServerAutosaveState(
        std::uint64_t nowMs,
        const ServerPropertiesConfig &serverProperties)
    {
        DedicatedServerAutosaveState autosaveState = {};
        autosaveState.intervalMs =
            GetDedicatedServerAutosaveIntervalMs(serverProperties);
        autosaveState.nextTickMs =
            ComputeNextDedicatedServerAutosaveDeadlineMs(
                nowMs,
                serverProperties);
        autosaveState.autosaveRequested = false;
        return autosaveState;
    }

    bool ShouldTriggerDedicatedServerAutosave(
        const DedicatedServerAutosaveState &autosaveState,
        std::uint64_t nowMs)
    {
        return nowMs >= autosaveState.nextTickMs;
    }

    void MarkDedicatedServerAutosaveRequested(
        DedicatedServerAutosaveState *autosaveState,
        std::uint64_t nowMs,
        const ServerPropertiesConfig &serverProperties)
    {
        if (autosaveState == nullptr)
        {
            return;
        }

        autosaveState->autosaveRequested = true;
        autosaveState->nextTickMs =
            ComputeNextDedicatedServerAutosaveDeadlineMs(
                nowMs,
                serverProperties);
    }

    void MarkDedicatedServerAutosaveCompleted(
        DedicatedServerAutosaveState *autosaveState)
    {
        if (autosaveState == nullptr)
        {
            return;
        }

        autosaveState->autosaveRequested = false;
    }

    DedicatedServerAutosaveLoopPlan BuildDedicatedServerAutosaveLoopPlan(
        const DedicatedServerAutosaveState &autosaveState,
        bool actionIdle,
        std::uint64_t nowMs)
    {
        DedicatedServerAutosaveLoopPlan plan = {};
        plan.shouldMarkCompleted =
            autosaveState.autosaveRequested && actionIdle;

        if (autosaveState.autosaveRequested &&
            !plan.shouldMarkCompleted)
        {
            return plan;
        }

        if (!ShouldTriggerDedicatedServerAutosave(
                autosaveState,
                nowMs))
        {
            return plan;
        }

        if (actionIdle)
        {
            plan.shouldRequestAutosave = true;
        }
        else
        {
            plan.shouldAdvanceDeadline = true;
        }

        return plan;
    }
}
