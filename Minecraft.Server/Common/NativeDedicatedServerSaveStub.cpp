#include "NativeDedicatedServerSaveStub.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace
{
    bool ParseBool(
        const std::string &text,
        bool *outValue)
    {
        if (outValue == nullptr)
        {
            return false;
        }

        if (text == "true")
        {
            *outValue = true;
            return true;
        }

        if (text == "false")
        {
            *outValue = false;
            return true;
        }

        return false;
    }

    bool ParseLongLong(
        const std::string &text,
        std::int64_t *outValue)
    {
        if (outValue == nullptr || text.empty())
        {
            return false;
        }

        errno = 0;
        char *end = nullptr;
        const long long parsed = std::strtoll(text.c_str(), &end, 10);
        if (text.c_str() == end || *end != '\0' || errno != 0)
        {
            return false;
        }

        *outValue = (std::int64_t)parsed;
        return true;
    }

    bool ParseUnsignedLongLong(
        const std::string &text,
        std::uint64_t *outValue)
    {
        if (outValue == nullptr || text.empty())
        {
            return false;
        }

        errno = 0;
        char *end = nullptr;
        const unsigned long long parsed = std::strtoull(
            text.c_str(),
            &end,
            10);
        if (text.c_str() == end || *end != '\0' || errno != 0)
        {
            return false;
        }

        *outValue = (std::uint64_t)parsed;
        return true;
    }

    bool ParseInt(
        const std::string &text,
        int *outValue)
    {
        if (outValue == nullptr || text.empty())
        {
            return false;
        }

        errno = 0;
        char *end = nullptr;
        const long parsed = std::strtol(text.c_str(), &end, 10);
        if (text.c_str() == end || *end != '\0' || errno != 0)
        {
            return false;
        }

        *outValue = (int)parsed;
        return true;
    }
}

namespace ServerRuntime
{
    bool BuildNativeDedicatedServerSaveStubText(
        const NativeDedicatedServerSaveStub &stub,
        std::string *outText)
    {
        if (outText == nullptr)
        {
            return false;
        }

        char buffer[1024] = {};
        const int written = std::snprintf(
            buffer,
            sizeof(buffer),
            "native-headless-save\n"
            "world=%s\n"
            "level-id=%s\n"
            "startup-mode=%s\n"
            "host=%s\n"
            "bind=%s\n"
            "payload-name=%s\n"
            "seed=%lld\n"
            "payload-bytes=%lld\n"
            "session-active=%s\n"
            "world-action=%s\n"
            "app-shutdown=%s\n"
            "gameplay-halted=%s\n"
            "configured-port=%d\n"
            "listener-port=%d\n"
            "public-slots=%u\n"
            "accepted-connections=%llu\n"
            "remote-commands=%llu\n"
            "autosave-requests=%llu\n"
            "autosave-completions=%llu\n"
            "tick-count=%llu\n"
            "uptime-ms=%llu\n"
            "saved-at-filetime=%llu\n",
            stub.worldName.c_str(),
            stub.levelId.c_str(),
            stub.startupMode.c_str(),
            stub.hostName.c_str(),
            stub.bindIp.c_str(),
            stub.payloadName.c_str(),
            (long long)stub.resolvedSeed,
            (long long)stub.payloadBytes,
            stub.sessionActive ? "true" : "false",
            stub.worldActionIdle ? "idle" : "busy",
            stub.appShutdownRequested ? "true" : "false",
            stub.gameplayHalted ? "true" : "false",
            stub.configuredPort,
            stub.listenerPort,
            stub.publicSlots,
            (unsigned long long)stub.acceptedConnections,
            (unsigned long long)stub.remoteCommands,
            (unsigned long long)stub.autosaveRequests,
            (unsigned long long)stub.autosaveCompletions,
            (unsigned long long)stub.platformTickCount,
            (unsigned long long)stub.uptimeMs,
            (unsigned long long)stub.savedAtFileTime);
        if (written <= 0 || written >= (int)sizeof(buffer))
        {
            outText->clear();
            return false;
        }

        *outText = buffer;
        return true;
    }

    bool ParseNativeDedicatedServerSaveStubText(
        const std::string &text,
        NativeDedicatedServerSaveStub *outStub)
    {
        if (outStub == nullptr)
        {
            return false;
        }

        *outStub = NativeDedicatedServerSaveStub{};
        size_t lineStart = 0;
        bool sawHeader = false;
        while (lineStart <= text.size())
        {
            const size_t lineEnd = text.find('\n', lineStart);
            std::string line = lineEnd == std::string::npos
                ? text.substr(lineStart)
                : text.substr(lineStart, lineEnd - lineStart);
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            if (!line.empty())
            {
                if (!sawHeader)
                {
                    if (line != "native-headless-save")
                    {
                        return false;
                    }
                    sawHeader = true;
                }
                else
                {
                    const size_t separator = line.find('=');
                    if (separator != std::string::npos)
                    {
                        const std::string key = line.substr(0, separator);
                        const std::string value = line.substr(separator + 1);
                        if (key == "world")
                        {
                            outStub->worldName = value;
                        }
                        else if (key == "level-id")
                        {
                            outStub->levelId = value;
                        }
                        else if (key == "startup-mode")
                        {
                            outStub->startupMode = value;
                        }
                        else if (key == "host")
                        {
                            outStub->hostName = value;
                        }
                        else if (key == "bind")
                        {
                            outStub->bindIp = value;
                        }
                        else if (key == "payload-name")
                        {
                            outStub->payloadName = value;
                        }
                        else if (key == "seed")
                        {
                            ParseLongLong(value, &outStub->resolvedSeed);
                        }
                        else if (key == "payload-bytes")
                        {
                            ParseLongLong(value, &outStub->payloadBytes);
                        }
                        else if (key == "session-active")
                        {
                            ParseBool(value, &outStub->sessionActive);
                        }
                        else if (key == "world-action")
                        {
                            outStub->worldActionIdle = value != "busy";
                        }
                        else if (key == "app-shutdown")
                        {
                            ParseBool(value, &outStub->appShutdownRequested);
                        }
                        else if (key == "gameplay-halted")
                        {
                            ParseBool(value, &outStub->gameplayHalted);
                        }
                        else if (key == "configured-port")
                        {
                            ParseInt(value, &outStub->configuredPort);
                        }
                        else if (key == "listener-port")
                        {
                            ParseInt(value, &outStub->listenerPort);
                        }
                        else if (key == "public-slots")
                        {
                            int publicSlots = 0;
                            if (ParseInt(value, &publicSlots) &&
                                publicSlots >= 0)
                            {
                                outStub->publicSlots =
                                    (unsigned int)publicSlots;
                            }
                        }
                        else if (key == "accepted-connections")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->acceptedConnections);
                        }
                        else if (key == "remote-commands")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->remoteCommands);
                        }
                        else if (key == "autosave-requests")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->autosaveRequests);
                        }
                        else if (key == "autosave-completions")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->autosaveCompletions);
                        }
                        else if (key == "tick-count")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->platformTickCount);
                        }
                        else if (key == "uptime-ms")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->uptimeMs);
                        }
                        else if (key == "saved-at-filetime")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->savedAtFileTime);
                        }
                    }
                }
            }

            if (lineEnd == std::string::npos)
            {
                break;
            }
            lineStart = lineEnd + 1;
        }

        return sawHeader &&
            !outStub->worldName.empty() &&
            !outStub->levelId.empty();
    }
}
