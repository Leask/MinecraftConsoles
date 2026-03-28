#include "ServerStoragePaths.h"

#include <cstdlib>

namespace ServerRuntime
{
    const char *GetServerStoragePlatformDirectory()
    {
#if defined(_NATIVE_DESKTOP)
        return "NativeDesktop";
#elif defined(_WINDOWS64)
        return "Windows64";
#elif defined(_DURANGO)
        return "Durango";
#elif defined(__ORBIS__)
        return "Orbis";
#elif defined(__PS3__)
        return "PS3";
#elif defined(__PSVITA__)
        return "PSVita";
#elif defined(_XBOX)
        return "Xbox";
#else
        return "UnknownPlatform";
#endif
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
