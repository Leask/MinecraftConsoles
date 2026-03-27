#pragma once

#include <string>
#include <vector>

#include "DedicatedServerRuntime.h"

namespace ServerRuntime
{
    struct DedicatedServerPlayerState
    {
        unsigned int smallId = 0;
        bool isRemote = false;
        bool isHostPlayer = false;
        std::wstring gamertag;
    };

    struct DedicatedServerPlatformState
    {
        std::string userNameUtf8;
        std::wstring userNameWide;
        std::string bindIp;
        int multiplayerPort = 0;
        int dedicatedServerPort = 0;
        bool multiplayerHost = false;
        bool multiplayerJoin = false;
        bool dedicatedServer = false;
        bool lanAdvertise = false;
        std::vector<DedicatedServerPlayerState> players;
    };

    DedicatedServerPlatformState BuildDedicatedServerPlatformState(
        const DedicatedServerRuntimeState &runtimeState,
        int playerCount);
}
