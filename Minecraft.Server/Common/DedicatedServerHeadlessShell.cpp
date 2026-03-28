#include "DedicatedServerHeadlessShell.h"

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>

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

    void AppendResponseLine(
        std::string *response,
        const std::string &line)
    {
        if (response == nullptr)
        {
            return;
        }

        response->append(line);
        response->push_back('\n');
    }

    void AppendStatusResponseLine(
        std::string *response,
        const ServerRuntime::DedicatedServerHeadlessShellContext &context,
        const ServerRuntime::DedicatedServerHeadlessShellState &state)
    {
        if (response == nullptr)
        {
            return;
        }

        const bool worldActionIdle =
            ServerRuntime::IsDedicatedServerWorldActionIdle(0);
        char buffer[512] = {};
        std::snprintf(
            buffer,
            sizeof(buffer),
            "status host=%s bind=%s configured-port=%d listener-port=%d "
            "accepted=%llu world-action=%s",
            context.hostName.c_str(),
            context.bindIp.c_str(),
            context.multiplayerPort,
            context.listenerPort,
            (unsigned long long)state.acceptedConnections,
            worldActionIdle ? "idle" : "busy");
        AppendResponseLine(response, buffer);

        std::snprintf(
            buffer,
            sizeof(buffer),
            "status world=%s level-id=%s storage=%s whitelist=%s lan=%s",
            context.worldName.c_str(),
            context.worldSaveId.c_str(),
            context.storageRoot.c_str(),
            context.whitelistEnabled ? "enabled" : "disabled",
            context.lanAdvertise ? "enabled" : "disabled");
        AppendResponseLine(response, buffer);
    }

    std::vector<std::string> SplitShellRequestLines(
        const std::string &request)
    {
        std::vector<std::string> lines;
        size_t start = 0;
        while (start < request.size())
        {
            const size_t end = request.find('\n', start);
            std::string line = end == std::string::npos
                ? request.substr(start)
                : request.substr(start, end - start);
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            lines.push_back(line);
            if (end == std::string::npos)
            {
                break;
            }

            start = end + 1;
        }

        return lines;
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
        const DedicatedServerHeadlessShellContext &context,
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

            std::string response;
            LceNetSetSocketRecvTimeout(acceptedSocket, 25);
            std::string request;
            char buffer[256] = {};
            for (;;)
            {
                const int received = LceNetRecv(
                    acceptedSocket,
                    buffer,
                    sizeof(buffer));
                if (received <= 0)
                {
                    break;
                }

                request.append(buffer, (size_t)received);
                if (request.size() >= 2048)
                {
                    break;
                }
            }

            const std::vector<std::string> requestLines =
                SplitShellRequestLines(request);
            if (requestLines.empty())
            {
                AppendResponseLine(
                    &response,
                    "ok connected; send newline-delimited commands");
            }

            for (size_t i = 0; i < requestLines.size(); ++i)
            {
                const std::string trimmedLine = TrimAscii(requestLines[i]);
                if (trimmedLine.empty())
                {
                    continue;
                }

                ++state->remoteCommands;
                LogInfof(
                    "console",
                    "remote shell command #%llu: %s",
                    (unsigned long long)state->remoteCommands,
                    trimmedLine.c_str());
                ExecuteDedicatedServerHeadlessShellCommand(
                    trimmedLine,
                    context,
                    *state,
                    &response);
            }

            if (!response.empty())
            {
                LceNetSendAll(
                    acceptedSocket,
                    response.data(),
                    (int)response.size());
            }
            LceNetCloseSocket(acceptedSocket);
        }
    }

    bool ExecuteDedicatedServerHeadlessShellCommand(
        const std::string &line,
        const DedicatedServerHeadlessShellContext &context,
        const DedicatedServerHeadlessShellState &state,
        std::string *response)
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
            AppendResponseLine(
                response,
                "commands: help, save, status, stop");
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
            AppendStatusResponseLine(response, context, state);
            return true;
        }

        if (command == "save")
        {
            RequestDedicatedServerWorldAutosave(0);
            LogInfo("console", "manual save requested");
            AppendResponseLine(response, "ok manual save requested");
            return true;
        }

        if (command == "stop")
        {
            LogInfo("console", "stop requested");
            RequestDedicatedServerShutdown();
            AppendResponseLine(response, "ok stop requested");
            return true;
        }

        LogWarnf(
            "console",
            "Unknown native shell command: %s",
            trimmedLine.c_str());
        AppendResponseLine(response, "error unknown command");
        return false;
    }
}
