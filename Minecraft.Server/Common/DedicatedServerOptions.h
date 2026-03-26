#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../ServerProperties.h"

namespace ServerRuntime
{
    struct DedicatedServerConfig
    {
        int port;
        char bindIP[256];
        char name[17];
        int maxPlayers;
        int worldSize;
        int worldSizeChunks;
        int worldHellScale;
        std::int64_t seed;
        EServerLogLevel logLevel;
        bool hasSeed;
        bool showHelp;
    };

    DedicatedServerConfig CreateDefaultDedicatedServerConfig();

    void ApplyServerPropertiesToDedicatedConfig(
        const ServerPropertiesConfig &serverProperties,
        DedicatedServerConfig *config);

    bool ParseDedicatedServerCommandLine(
        int argc,
        char **argv,
        DedicatedServerConfig *config,
        std::string *outError);

    void BuildDedicatedServerUsageLines(std::vector<std::string> *outLines);
}
