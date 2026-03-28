#include "DedicatedServerHeadlessShell.h"

#include <cctype>
#include <string>

#include "DedicatedServerPlatformRuntime.h"
#include "../ServerLogger.h"
#include "DedicatedServerSignalState.h"
#include "StringUtils.h"

namespace
{
    std::string TrimAscii(const std::string &value)
    {
        size_t start = 0;
        while (start < value.size() &&
            std::isspace((unsigned char)value[start]))
        {
            ++start;
        }

        size_t end = value.size();
        while (end > start &&
            std::isspace((unsigned char)value[end - 1]))
        {
            --end;
        }

        return value.substr(start, end - start);
    }

    std::string ToLowerAscii(std::string value)
    {
        for (size_t i = 0; i < value.size(); ++i)
        {
            value[i] = (char)std::tolower((unsigned char)value[i]);
        }
        return value;
    }
}

namespace ServerRuntime
{
    DedicatedServerHeadlessShellContext
    BuildDedicatedServerHeadlessShellContext(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        int listenerPort)
    {
        DedicatedServerHeadlessShellContext shellContext = {};
        shellContext.storageRoot = context.storageRoot;
        shellContext.worldName = StringUtils::WideToUtf8(
            context.serverProperties.worldName);
        shellContext.worldSaveId = context.serverProperties.worldSaveId;
        shellContext.bindIp = platformState.bindIp;
        shellContext.hostName = platformState.userNameUtf8;
        shellContext.multiplayerPort = platformState.multiplayerPort;
        shellContext.listenerPort = listenerPort;
        shellContext.whitelistEnabled =
            context.serverProperties.whiteListEnabled;
        shellContext.lanAdvertise = context.serverProperties.lanAdvertise;
        return shellContext;
    }

    void LogDedicatedServerHeadlessShellHelp()
    {
        LogInfo("console", "native shell commands: help, save, status, stop");
    }

    void PollDedicatedServerHeadlessShellConnections(
        LceSocketHandle listener,
        DedicatedServerHeadlessShellState *state)
    {
        if (listener == LCE_INVALID_SOCKET || state == nullptr)
        {
            return;
        }

        for (;;)
        {
            char acceptedIp[64] = {};
            int acceptedPort = -1;
            const LceSocketHandle acceptedSocket = LceNetAcceptIpv4(
                listener,
                acceptedIp,
                sizeof(acceptedIp),
                &acceptedPort);
            if (acceptedSocket == LCE_INVALID_SOCKET)
            {
                break;
            }

            ++state->acceptedConnections;
            LogInfof(
                "network",
                "native shell accepted connection #%llu from %s:%d",
                (unsigned long long)state->acceptedConnections,
                acceptedIp,
                acceptedPort);
            LceNetCloseSocket(acceptedSocket);
        }
    }

    bool ExecuteDedicatedServerHeadlessShellCommand(
        const std::string &line,
        const DedicatedServerHeadlessShellContext &context,
        const DedicatedServerHeadlessShellState &state)
    {
        const std::string trimmedLine = TrimAscii(line);
        if (trimmedLine.empty())
        {
            return true;
        }

        const std::string command = ToLowerAscii(trimmedLine);
        if (command == "help")
        {
            LogDedicatedServerHeadlessShellHelp();
            return true;
        }

        if (command == "status")
        {
            const bool worldActionIdle =
                IsDedicatedServerWorldActionIdle(0);
            LogInfof(
                "console",
                "status host=%s bind=%s configured-port=%d listener-port=%d accepted=%llu world-action=%s",
                context.hostName.c_str(),
                context.bindIp.c_str(),
                context.multiplayerPort,
                context.listenerPort,
                (unsigned long long)state.acceptedConnections,
                worldActionIdle ? "idle" : "busy");
            LogInfof(
                "console",
                "status world=%s level-id=%s storage=%s whitelist=%s lan=%s",
                context.worldName.c_str(),
                context.worldSaveId.c_str(),
                context.storageRoot.c_str(),
                context.whitelistEnabled ? "enabled" : "disabled",
                context.lanAdvertise ? "enabled" : "disabled");
            return true;
        }

        if (command == "save")
        {
            RequestDedicatedServerWorldAutosave(0);
            LogInfo("console", "manual save requested");
            return true;
        }

        if (command == "stop")
        {
            LogInfo("console", "stop requested");
            RequestDedicatedServerShutdown();
            return true;
        }

        LogWarnf(
            "console",
            "Unknown native shell command: %s",
            trimmedLine.c_str());
        return false;
    }
}
