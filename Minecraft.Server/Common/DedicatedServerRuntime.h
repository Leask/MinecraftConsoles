#pragma once

#include <cstdint>
#include <string>

#include "../ServerProperties.h"
#include "DedicatedServerOptions.h"

namespace ServerRuntime
{
    struct DedicatedServerRuntimeState
    {
        std::string hostNameUtf8;
        std::wstring hostNameWide;
        std::string bindIp;
        int multiplayerPort;
        int dedicatedServerPort;
        bool multiplayerHost;
        bool multiplayerJoin;
        bool dedicatedServer;
        bool lanAdvertise;
    };

    struct DedicatedServerAutosaveState
    {
        std::uint64_t intervalMs;
        std::uint64_t nextTickMs;
        bool autosaveRequested;
    };

    DedicatedServerRuntimeState BuildDedicatedServerRuntimeState(
        const DedicatedServerConfig &config,
        const ServerPropertiesConfig &serverProperties);

    std::uint64_t GetDedicatedServerAutosaveIntervalMs(
        const ServerPropertiesConfig &serverProperties);

    std::uint64_t ComputeNextDedicatedServerAutosaveDeadlineMs(
        std::uint64_t nowMs,
        const ServerPropertiesConfig &serverProperties);

    DedicatedServerAutosaveState CreateDedicatedServerAutosaveState(
        std::uint64_t nowMs,
        const ServerPropertiesConfig &serverProperties);

    bool ShouldTriggerDedicatedServerAutosave(
        const DedicatedServerAutosaveState &autosaveState,
        std::uint64_t nowMs);

    void MarkDedicatedServerAutosaveRequested(
        DedicatedServerAutosaveState *autosaveState,
        std::uint64_t nowMs,
        const ServerPropertiesConfig &serverProperties);

    void MarkDedicatedServerAutosaveCompleted(
        DedicatedServerAutosaveState *autosaveState);
}
