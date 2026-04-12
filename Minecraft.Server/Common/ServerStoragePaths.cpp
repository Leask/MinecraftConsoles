#include "ServerStoragePaths.h"

#include <cstdlib>

namespace ServerRuntime
{
    const char *GetServerStoragePlatformDirectory()
    {
        return "NativeDesktop";
    }

    std::string GetServerStorageRootOverridePath()
    {
        const char *overrideRoot = std::getenv(
            "MINECRAFT_SERVER_STORAGE_ROOT");
        if (overrideRoot == nullptr || overrideRoot[0] == '\0')
        {
            return std::string();
        }

        return std::string(overrideRoot);
    }

    std::string GetServerGameHddRootPath()
    {
        const std::string overrideRoot = GetServerStorageRootOverridePath();
        if (!overrideRoot.empty())
        {
            return overrideRoot;
        }

        std::string path = GetServerStoragePlatformDirectory();
        path += "/GameHDD";
        return path;
    }
}
