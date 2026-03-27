#include "DedicatedServerPlatformState.h"

#include <string>

namespace
{
    std::wstring BuildDedicatedPlayerGamertag(
        unsigned int playerIndex,
        const std::wstring &hostName)
    {
        if (playerIndex == 0)
        {
            return hostName;
        }

        return std::wstring(L"Player") + std::to_wstring(playerIndex);
    }
}

namespace ServerRuntime
{
    DedicatedServerPlatformState BuildDedicatedServerPlatformState(
        const DedicatedServerRuntimeState &runtimeState,
        int playerCount)
    {
        DedicatedServerPlatformState platformState = {};
        platformState.userNameUtf8 = runtimeState.hostNameUtf8.empty()
            ? "DedicatedServer"
            : runtimeState.hostNameUtf8;
        platformState.userNameWide = runtimeState.hostNameWide.empty()
            ? L"DedicatedServer"
            : runtimeState.hostNameWide;
        platformState.bindIp = runtimeState.bindIp;
        platformState.multiplayerPort = runtimeState.multiplayerPort;
        platformState.dedicatedServerPort = runtimeState.dedicatedServerPort;
        platformState.multiplayerHost = runtimeState.multiplayerHost;
        platformState.multiplayerJoin = runtimeState.multiplayerJoin;
        platformState.dedicatedServer = runtimeState.dedicatedServer;
        platformState.lanAdvertise = runtimeState.lanAdvertise;

        const int resolvedPlayerCount = playerCount > 0 ? playerCount : 1;
        platformState.players.reserve(static_cast<std::size_t>(resolvedPlayerCount));
        for (int playerIndex = 0; playerIndex < resolvedPlayerCount; ++playerIndex)
        {
            DedicatedServerPlayerState playerState = {};
            playerState.smallId = static_cast<unsigned int>(playerIndex);
            playerState.isRemote = false;
            playerState.isHostPlayer = playerIndex == 0;
            playerState.gamertag = BuildDedicatedPlayerGamertag(
                static_cast<unsigned int>(playerIndex),
                platformState.userNameWide);
            platformState.players.push_back(playerState);
        }

        return platformState;
    }
}
