#include "DedicatedServerSessionConfig.h"

#include "Minecraft.Client/Common/App_Defines.h"

namespace
{
    void SetFlag(unsigned int *hostSettings, unsigned int mask, bool enabled)
    {
        if (hostSettings == nullptr)
        {
            return;
        }

        if (enabled)
        {
            *hostSettings |= mask;
        }
        else
        {
            *hostSettings &= ~mask;
        }
    }

    void SetInvertedFlag(
        unsigned int *hostSettings,
        unsigned int mask,
        bool enabled)
    {
        SetFlag(hostSettings, mask, !enabled);
    }

    void SetMaskedValue(
        unsigned int *hostSettings,
        unsigned int mask,
        unsigned int value)
    {
        if (hostSettings == nullptr)
        {
            return;
        }

        *hostSettings &= ~mask;
        *hostSettings |= (mask & value);
    }
}

namespace ServerRuntime
{
    DedicatedServerSessionConfig BuildDedicatedServerSessionConfig(
        const DedicatedServerConfig &config,
        const ServerPropertiesConfig &serverProperties)
    {
        DedicatedServerSessionConfig sessionConfig = {};
        SetMaskedValue(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_DIFFICULTY,
            static_cast<unsigned int>(serverProperties.difficulty));
        SetMaskedValue(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_GAMETYPE,
            static_cast<unsigned int>(serverProperties.gameMode) << 4U);
        SetMaskedValue(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_WORLDSIZE,
            static_cast<unsigned int>(config.worldSize)
                << GAME_HOST_OPTION_BITMASK_WORLDSIZE_BITSHIFT);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS,
            serverProperties.friendsOfFriends);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_GAMERTAGS,
            serverProperties.gamertags);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_LEVELTYPE,
            serverProperties.levelTypeFlat);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_STRUCTURES,
            serverProperties.generateStructures);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_BONUSCHEST,
            serverProperties.bonusChest);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_PVP,
            serverProperties.pvp);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS,
            serverProperties.trustPlayers);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_TNT,
            serverProperties.tnt);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_FIRESPREADS,
            serverProperties.fireSpreads);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_HOSTFLY,
            serverProperties.hostCanFly);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_HOSTHUNGER,
            serverProperties.hostCanChangeHunger);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE,
            serverProperties.hostCanBeInvisible);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_BEDROCKFOG,
            serverProperties.bedrockFog);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_DISABLESAVE,
            serverProperties.disableSaving);
        SetFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_KEEPINVENTORY,
            serverProperties.keepInventory);
        SetInvertedFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_MOBGRIEFING,
            serverProperties.mobGriefing);
        SetInvertedFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING,
            serverProperties.doMobSpawning);
        SetInvertedFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_DOMOBLOOT,
            serverProperties.doMobLoot);
        SetInvertedFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_DOTILEDROPS,
            serverProperties.doTileDrops);
        SetInvertedFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_NATURALREGEN,
            serverProperties.naturalRegeneration);
        SetInvertedFlag(
            &sessionConfig.hostSettings,
            GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE,
            serverProperties.doDaylightCycle);

        sessionConfig.saveDisabled = serverProperties.disableSaving;
        sessionConfig.maxPlayers = config.maxPlayers;
        sessionConfig.networkMaxPlayers =
            static_cast<unsigned char>(
                (config.maxPlayers < 255) ? config.maxPlayers : 255);
        sessionConfig.hasSeed = config.hasSeed;
        sessionConfig.seed = config.seed;
        sessionConfig.worldSizeChunks =
            static_cast<unsigned int>(config.worldSizeChunks);
        sessionConfig.worldHellScale =
            static_cast<unsigned char>(config.worldHellScale);
        return sessionConfig;
    }

    DedicatedServerAppSessionPlan BuildDedicatedServerAppSessionPlan(
        const DedicatedServerSessionConfig &sessionConfig)
    {
        DedicatedServerAppSessionPlan plan = {};
        plan.shouldInitGameSettings = true;
        plan.tutorialMode = false;
        plan.corruptSaveDeleted = false;
        plan.hostSettings = sessionConfig.hostSettings;
        plan.shouldApplyWorldSize = true;
        plan.worldSizeChunks = sessionConfig.worldSizeChunks;
        plan.worldHellScale = sessionConfig.worldHellScale;
        plan.saveDisabled = sessionConfig.saveDisabled;
        return plan;
    }
}
