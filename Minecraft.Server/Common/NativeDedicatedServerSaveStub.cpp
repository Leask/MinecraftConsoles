#include "NativeDedicatedServerSaveStub.h"

#include <cerrno>
#include <climits>
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

    bool ParseUnsignedInt(
        const std::string &text,
        unsigned int *outValue)
    {
        if (outValue == nullptr || text.empty())
        {
            return false;
        }

        errno = 0;
        char *end = nullptr;
        const unsigned long parsed = std::strtoul(
            text.c_str(),
            &end,
            10);
        if (text.c_str() == end || *end != '\0' || errno != 0 ||
            parsed > UINT_MAX)
        {
            return false;
        }

        *outValue = (unsigned int)parsed;
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

        char buffer[4096] = {};
        const int written = std::snprintf(
            buffer,
            sizeof(buffer),
            "native-headless-save\n"
            "world=%s\n"
            "level-id=%s\n"
            "startup-mode=%s\n"
            "session-phase=%s\n"
            "host=%s\n"
            "bind=%s\n"
            "payload-name=%s\n"
            "seed=%lld\n"
            "payload-bytes=%lld\n"
            "payload-checksum=%llu\n"
            "host-settings=%u\n"
            "dedicated-no-local-player=%s\n"
            "world-size-chunks=%u\n"
            "hell-scale=%u\n"
            "session-active=%s\n"
            "world-action=%s\n"
            "app-shutdown=%s\n"
            "gameplay-halted=%s\n"
            "initial-save-requested=%s\n"
            "initial-save-completed=%s\n"
            "initial-save-timed-out=%s\n"
            "session-completed=%s\n"
            "requested-app-shutdown=%s\n"
            "shutdown-halted=%s\n"
            "configured-port=%d\n"
            "listener-port=%d\n"
            "public-slots=%u\n"
            "startup-payload-present=%s\n"
            "startup-payload-validated=%s\n"
            "startup-thread-iterations=%llu\n"
            "startup-thread-duration-ms=%llu\n"
            "accepted-connections=%llu\n"
            "remote-commands=%llu\n"
            "autosave-requests=%llu\n"
            "autosave-completions=%llu\n"
            "tick-count=%llu\n"
            "uptime-ms=%llu\n"
            "gameplay-iterations=%llu\n"
            "saved-at-filetime=%llu\n",
            stub.worldName.c_str(),
            stub.levelId.c_str(),
            stub.startupMode.c_str(),
            stub.sessionPhase.c_str(),
            stub.hostName.c_str(),
            stub.bindIp.c_str(),
            stub.payloadName.c_str(),
            (long long)stub.resolvedSeed,
            (long long)stub.payloadBytes,
            (unsigned long long)stub.payloadChecksum,
            stub.hostSettings,
            stub.dedicatedNoLocalHostPlayer ? "true" : "false",
            stub.worldSizeChunks,
            stub.worldHellScale,
            stub.sessionActive ? "true" : "false",
            stub.worldActionIdle ? "idle" : "busy",
            stub.appShutdownRequested ? "true" : "false",
            stub.gameplayHalted ? "true" : "false",
            stub.initialSaveRequested ? "true" : "false",
            stub.initialSaveCompleted ? "true" : "false",
            stub.initialSaveTimedOut ? "true" : "false",
            stub.sessionCompleted ? "true" : "false",
            stub.requestedAppShutdown ? "true" : "false",
            stub.shutdownHaltedGameplay ? "true" : "false",
            stub.configuredPort,
            stub.listenerPort,
            stub.publicSlots,
            stub.startupPayloadPresent ? "true" : "false",
            stub.startupPayloadValidated ? "true" : "false",
            (unsigned long long)stub.startupThreadIterations,
            (unsigned long long)stub.startupThreadDurationMs,
            (unsigned long long)stub.acceptedConnections,
            (unsigned long long)stub.remoteCommands,
            (unsigned long long)stub.autosaveRequests,
            (unsigned long long)stub.autosaveCompletions,
            (unsigned long long)stub.platformTickCount,
            (unsigned long long)stub.uptimeMs,
            (unsigned long long)stub.gameplayLoopIterations,
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
                        else if (key == "session-phase")
                        {
                            outStub->sessionPhase = value;
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
                        else if (key == "payload-checksum")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->payloadChecksum);
                        }
                        else if (key == "host-settings")
                        {
                            ParseUnsignedInt(value, &outStub->hostSettings);
                        }
                        else if (key == "dedicated-no-local-player")
                        {
                            ParseBool(
                                value,
                                &outStub->dedicatedNoLocalHostPlayer);
                        }
                        else if (key == "world-size-chunks")
                        {
                            ParseUnsignedInt(
                                value,
                                &outStub->worldSizeChunks);
                        }
                        else if (key == "hell-scale")
                        {
                            ParseUnsignedInt(
                                value,
                                &outStub->worldHellScale);
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
                        else if (key == "initial-save-requested")
                        {
                            ParseBool(value, &outStub->initialSaveRequested);
                        }
                        else if (key == "initial-save-completed")
                        {
                            ParseBool(value, &outStub->initialSaveCompleted);
                        }
                        else if (key == "initial-save-timed-out")
                        {
                            ParseBool(value, &outStub->initialSaveTimedOut);
                        }
                        else if (key == "session-completed")
                        {
                            ParseBool(value, &outStub->sessionCompleted);
                        }
                        else if (key == "requested-app-shutdown")
                        {
                            ParseBool(value, &outStub->requestedAppShutdown);
                        }
                        else if (key == "shutdown-halted")
                        {
                            ParseBool(value, &outStub->shutdownHaltedGameplay);
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
                        else if (key == "startup-payload-present")
                        {
                            ParseBool(
                                value,
                                &outStub->startupPayloadPresent);
                        }
                        else if (key == "startup-payload-validated")
                        {
                            ParseBool(
                                value,
                                &outStub->startupPayloadValidated);
                        }
                        else if (key == "startup-thread-iterations")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->startupThreadIterations);
                        }
                        else if (key == "startup-thread-duration-ms")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->startupThreadDurationMs);
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
                        else if (key == "gameplay-iterations")
                        {
                            ParseUnsignedLongLong(
                                value,
                                &outStub->gameplayLoopIterations);
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
