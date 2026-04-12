#include "ServerPropertiesSmoke.h"

#include <filesystem>
#include <string>
#include <vector>

#include "Minecraft.Server/Access/WhitelistManager.h"
#include "Minecraft.Server/Common/FileUtils.h"
#include "Minecraft.Server/ServerProperties.h"

namespace
{
    class ScopedCurrentDirectory
    {
    public:
        explicit ScopedCurrentDirectory(const std::filesystem::path& path)
            : m_previous(std::filesystem::current_path())
            , m_active(false)
        {
            std::error_code errorCode;
            std::filesystem::remove_all(path, errorCode);
            errorCode.clear();
            std::filesystem::create_directories(path, errorCode);
            std::filesystem::current_path(path, errorCode);
            m_active = !errorCode;
        }

        ~ScopedCurrentDirectory()
        {
            if (!m_active)
            {
                return;
            }

            std::error_code errorCode;
            std::filesystem::current_path(m_previous, errorCode);
        }

        bool IsActive() const
        {
            return m_active;
        }

    private:
        std::filesystem::path m_previous;
        bool m_active;
    };
}

ServerPropertiesSmokeResult RunServerPropertiesSmoke()
{
    ServerPropertiesSmokeResult result = {};

    ScopedCurrentDirectory directoryGuard(
        "build/portability-server-properties-smoke");
    if (!directoryGuard.IsActive())
    {
        return result;
    }

    std::error_code cleanupError;
    std::filesystem::remove("server.properties", cleanupError);
    cleanupError.clear();
    std::filesystem::remove("whitelist.json", cleanupError);

    const ServerRuntime::ServerPropertiesConfig defaults =
        ServerRuntime::LoadServerPropertiesConfig();
    std::string defaultText;
    const bool readDefaultText =
        ServerRuntime::FileUtils::ReadTextFile(
            "server.properties",
            &defaultText);
    result.createdDefaults =
        readDefaultText &&
        defaults.serverPort == 25565 &&
        defaults.serverName == "DedicatedServer" &&
        defaults.worldSaveId == "world" &&
        defaultText.find("server-port=25565") != std::string::npos;

    const std::string messyText =
        "server-port=70000\n"
        "server-name=Native Smoke Host Name\n"
        "level-name=Fancy Native World\n"
        "level-id=Fancy Native World !!!\n"
        "white-list=YES\n"
        "autosave-interval=2\n"
        "world-size=medium\n";
    ServerRuntime::FileUtils::WriteTextFileAtomic(
        "server.properties",
        messyText);

    const ServerRuntime::ServerPropertiesConfig normalized =
        ServerRuntime::LoadServerPropertiesConfig();
    std::string normalizedText;
    const bool readNormalizedText =
        ServerRuntime::FileUtils::ReadTextFile(
            "server.properties",
            &normalizedText);
    result.normalizedValues =
        readNormalizedText &&
        normalized.serverPort == 65535 &&
        normalized.serverName == "Native Smoke Hos" &&
        normalized.whiteListEnabled &&
        normalized.autosaveIntervalSeconds == 5 &&
        normalized.worldSizeChunks == 3 * 64 &&
        normalized.worldHellScale == 6 &&
        normalized.worldSaveId == "fancy_native_world____" &&
        normalizedText.find("server-port=65535") != std::string::npos &&
        normalizedText.find("white-list=true") != std::string::npos &&
        normalizedText.find("autosave-interval=5") != std::string::npos;

    ServerRuntime::ServerPropertiesConfig saved = normalized;
    saved.worldName = L"Persisted World";
    saved.worldSaveId = "Persisted Save";
    saved.whiteListEnabled = false;
    result.savePersisted = ServerRuntime::SaveServerPropertiesConfig(saved);
    if (result.savePersisted)
    {
        std::string savedText;
        result.savePersisted =
            ServerRuntime::FileUtils::ReadTextFile(
                "server.properties",
                &savedText) &&
            savedText.find("level-name=Persisted World") != std::string::npos &&
            savedText.find("level-id=persisted_save") != std::string::npos &&
            savedText.find("white-list=false") != std::string::npos;
    }

    ServerRuntime::Access::WhitelistManager whitelistManager(".");
    const bool ensuredWhitelist = whitelistManager.EnsureWhitelistFileExists();
    const bool reloadedWhitelist = whitelistManager.Reload();
    ServerRuntime::Access::WhitelistedPlayerEntry whitelistEntry = {};
    whitelistEntry.xuid = "12345";
    whitelistEntry.name = "SmokeHost";
    whitelistEntry.metadata =
        ServerRuntime::Access::WhitelistManager::BuildDefaultMetadata(
            "smoke");
    const bool addedWhitelist = whitelistManager.AddPlayer(whitelistEntry);
    const bool whitelistedPlayer =
        whitelistManager.IsPlayerWhitelistedByXuid("12345");
    std::vector<ServerRuntime::Access::WhitelistedPlayerEntry> whitelistSnapshot;
    const bool snapshotWhitelist =
        whitelistManager.SnapshotWhitelistedPlayers(&whitelistSnapshot);
    result.whitelistWorkflowOk =
        ensuredWhitelist &&
        reloadedWhitelist &&
        addedWhitelist &&
        whitelistedPlayer &&
        snapshotWhitelist &&
        whitelistSnapshot.size() == 1;

    return result;
}
