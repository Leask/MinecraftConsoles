#include "DedicatedServerOptions.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>

namespace
{
    static constexpr int kDefaultServerPort = 25565;
    static constexpr int kDefaultMaxPlayers = 256;
    static constexpr int kClassicWorldSize = 1;
    static constexpr int kClassicWorldSizeChunks = 54;
    static constexpr int kClassicWorldHellScale = 3;

    bool EqualsIgnoreCase(const char *lhs, const char *rhs)
    {
        if (lhs == NULL || rhs == NULL)
        {
            return false;
        }

        return strcasecmp(lhs, rhs) == 0;
    }

    void CopyString(char *destination, std::size_t count, const char *source)
    {
        if (destination == NULL || count == 0)
        {
            return;
        }

        destination[0] = 0;
        if (source == NULL)
        {
            return;
        }

        std::snprintf(destination, count, "%s", source);
    }

    bool ParseIntArg(const char *value, int *outValue)
    {
        if (value == NULL || *value == 0 || outValue == NULL)
        {
            return false;
        }

        char *end = NULL;
        long parsed = std::strtol(value, &end, 10);
        if (end == value || *end != 0)
        {
            return false;
        }

        *outValue = static_cast<int>(parsed);
        return true;
    }

    bool ParseInt64Arg(const char *value, std::int64_t *outValue)
    {
        if (value == NULL || *value == 0 || outValue == NULL)
        {
            return false;
        }

        char *end = NULL;
        long long parsed = std::strtoll(value, &end, 10);
        if (end == value || *end != 0)
        {
            return false;
        }

        *outValue = static_cast<std::int64_t>(parsed);
        return true;
    }
}

namespace ServerRuntime
{
    DedicatedServerConfig CreateDefaultDedicatedServerConfig()
    {
        DedicatedServerConfig config = {};
        config.port = kDefaultServerPort;
        CopyString(config.bindIP, sizeof(config.bindIP), "0.0.0.0");
        CopyString(config.name, sizeof(config.name), "DedicatedServer");
        config.maxPlayers = kDefaultMaxPlayers;
        config.worldSize = kClassicWorldSize;
        config.worldSizeChunks = kClassicWorldSizeChunks;
        config.worldHellScale = kClassicWorldHellScale;
        config.seed = 0;
        config.logLevel = eServerLogLevel_Info;
        config.hasSeed = false;
        config.showHelp = false;
        return config;
    }

    void ApplyServerPropertiesToDedicatedConfig(
        const ServerPropertiesConfig &serverProperties,
        DedicatedServerConfig *config)
    {
        if (config == NULL)
        {
            return;
        }

        config->port = serverProperties.serverPort;
        CopyString(
            config->bindIP,
            sizeof(config->bindIP),
            serverProperties.serverIp.empty()
                ? "0.0.0.0"
                : serverProperties.serverIp.c_str());
        CopyString(
            config->name,
            sizeof(config->name),
            serverProperties.serverName.empty()
                ? "DedicatedServer"
                : serverProperties.serverName.c_str());
        config->maxPlayers = serverProperties.maxPlayers;
        config->worldSize = serverProperties.worldSize;
        config->worldSizeChunks = serverProperties.worldSizeChunks;
        config->worldHellScale = serverProperties.worldHellScale;
        config->logLevel = serverProperties.logLevel;
        config->hasSeed = serverProperties.hasSeed;
        config->seed = serverProperties.seed;
    }

    bool ParseDedicatedServerCommandLine(
        int argc,
        char **argv,
        DedicatedServerConfig *config,
        std::string *outError)
    {
        if (outError != NULL)
        {
            outError->clear();
        }

        if (config == NULL)
        {
            if (outError != NULL)
            {
                *outError = "Server config output is null.";
            }
            return false;
        }

        for (int i = 1; i < argc; ++i)
        {
            const char *arg = argv[i];
            if (EqualsIgnoreCase(arg, "-help") ||
                EqualsIgnoreCase(arg, "--help") ||
                EqualsIgnoreCase(arg, "-h"))
            {
                config->showHelp = true;
                return true;
            }
            else if (EqualsIgnoreCase(arg, "-port") && (i + 1 < argc))
            {
                int port = 0;
                if (!ParseIntArg(argv[++i], &port) || port < 0 || port > 65535)
                {
                    if (outError != NULL)
                    {
                        *outError = "Invalid -port value.";
                    }
                    return false;
                }
                config->port = port;
            }
            else if ((EqualsIgnoreCase(arg, "-ip") ||
                    EqualsIgnoreCase(arg, "-bind")) &&
                (i + 1 < argc))
            {
                CopyString(config->bindIP, sizeof(config->bindIP), argv[++i]);
            }
            else if (EqualsIgnoreCase(arg, "-name") && (i + 1 < argc))
            {
                CopyString(config->name, sizeof(config->name), argv[++i]);
            }
            else if (EqualsIgnoreCase(arg, "-maxplayers") && (i + 1 < argc))
            {
                int maxPlayers = 0;
                if (!ParseIntArg(argv[++i], &maxPlayers) ||
                    maxPlayers <= 0 ||
                    maxPlayers > kDefaultMaxPlayers)
                {
                    if (outError != NULL)
                    {
                        *outError = "Invalid -maxplayers value.";
                    }
                    return false;
                }
                config->maxPlayers = maxPlayers;
            }
            else if (EqualsIgnoreCase(arg, "-seed") && (i + 1 < argc))
            {
                if (!ParseInt64Arg(argv[++i], &config->seed))
                {
                    if (outError != NULL)
                    {
                        *outError = "Invalid -seed value.";
                    }
                    return false;
                }
                config->hasSeed = true;
            }
            else if (EqualsIgnoreCase(arg, "-loglevel") && (i + 1 < argc))
            {
                if (!TryParseServerLogLevel(argv[++i], &config->logLevel))
                {
                    if (outError != NULL)
                    {
                        *outError =
                            "Invalid -loglevel value. Use debug/info/warn/error.";
                    }
                    return false;
                }
            }
            else
            {
                if (outError != NULL)
                {
                    *outError = std::string("Unknown or incomplete argument: ") +
                        (arg != NULL ? arg : "(null)");
                }
                return false;
            }
        }

        return true;
    }

    void BuildDedicatedServerUsageLines(std::vector<std::string> *outLines)
    {
        if (outLines == NULL)
        {
            return;
        }

        outLines->clear();
        outLines->push_back("Minecraft.Server [options]");
        outLines->push_back(
            "  -port <0-65535>       Listen TCP port "
            "(0 = ephemeral, default: server.properties:server-port)");
        outLines->push_back(
            "  -ip <addr>            Bind address "
            "(default: server.properties:server-ip)");
        outLines->push_back("  -bind <addr>          Alias of -ip");
        outLines->push_back(
            "  -name <name>          Host display name (max 16 chars, "
            "default: server.properties:server-name)");
        outLines->push_back(
            "  -maxplayers <1-256>   Public slots "
            "(default: server.properties:max-players)");
        outLines->push_back(
            "  -seed <int64>         World seed "
            "(overrides server.properties:level-seed)");
        outLines->push_back(
            "  -loglevel <level>     debug|info|warn|error "
            "(default: server.properties:log-level)");
        outLines->push_back("  -help                 Show this help");
    }
}
