#include "ServerStoragePaths.h"

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

    std::string GetServerGameHddRootPath()
    {
        std::string path = GetServerStoragePlatformDirectory();
        path += "/GameHDD";
        return path;
    }
}
