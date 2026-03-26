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

    DedicatedServerRuntimeState BuildDedicatedServerRuntimeState(
        const DedicatedServerConfig &config,
        const ServerPropertiesConfig &serverProperties);

    std::uint64_t GetDedicatedServerAutosaveIntervalMs(
        const ServerPropertiesConfig &serverProperties);

    std::uint64_t ComputeNextDedicatedServerAutosaveDeadlineMs(
        std::uint64_t nowMs,
        const ServerPropertiesConfig &serverProperties);
}
