#pragma once

#include <cstdint>

#include "../ServerProperties.h"
#include "DedicatedServerOptions.h"

namespace ServerRuntime
{
    struct DedicatedServerSessionConfig
    {
        unsigned int hostSettings = 0;
        bool saveDisabled = false;
        int maxPlayers = 0;
        unsigned char networkMaxPlayers = 0;
        bool hasSeed = false;
        std::int64_t seed = 0;
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
    };

    struct DedicatedServerAppSessionPlan
    {
        bool shouldInitGameSettings = true;
        bool tutorialMode = false;
        bool corruptSaveDeleted = false;
        unsigned int hostSettings = 0;
        bool shouldApplyWorldSize = true;
        unsigned int worldSizeChunks = 0;
        unsigned char worldHellScale = 0;
        bool saveDisabled = false;
    };

    DedicatedServerSessionConfig BuildDedicatedServerSessionConfig(
        const DedicatedServerConfig &config,
        const ServerPropertiesConfig &serverProperties);

    DedicatedServerAppSessionPlan BuildDedicatedServerAppSessionPlan(
        const DedicatedServerSessionConfig &sessionConfig);
}
