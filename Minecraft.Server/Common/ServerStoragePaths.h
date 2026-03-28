#pragma once

#include <string>

namespace ServerRuntime
{
    const char *GetServerStoragePlatformDirectory();
    std::string GetServerStorageRootOverridePath();
    std::string GetServerGameHddRootPath();
}
