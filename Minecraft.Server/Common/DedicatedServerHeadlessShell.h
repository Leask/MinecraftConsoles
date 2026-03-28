#pragma once

#include <cstdint>
#include <string>

#include "DedicatedServerBootstrap.h"
#include "DedicatedServerPlatformState.h"
#include "lce_net/lce_net.h"

namespace ServerRuntime
{
    struct DedicatedServerHeadlessShellContext
    {
        std::string storageRoot;
        std::string worldName;
        std::string worldSaveId;
        std::string bindIp;
        std::string hostName;
        int multiplayerPort = 0;
        int listenerPort = 0;
        bool whitelistEnabled = false;
        bool lanAdvertise = false;
    };

    struct DedicatedServerHeadlessShellState
    {
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
    };

    DedicatedServerHeadlessShellContext
    BuildDedicatedServerHeadlessShellContext(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        int listenerPort);

    void LogDedicatedServerHeadlessShellHelp();

    void PollDedicatedServerHeadlessShellConnections(
        LceSocketHandle listener,
        const DedicatedServerHeadlessShellContext &context,
        DedicatedServerHeadlessShellState *state);

    bool ExecuteDedicatedServerHeadlessShellCommand(
        const std::string &line,
        const DedicatedServerHeadlessShellContext &context,
        const DedicatedServerHeadlessShellState &state,
        std::string *response = nullptr);
}
