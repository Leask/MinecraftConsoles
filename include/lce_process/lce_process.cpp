#include "lce_process.h"

#include <filesystem>
#include <string>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <vector>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace
{
    bool GetExecutablePath(std::filesystem::path* outPath)
    {
        if (outPath == nullptr)
        {
            return false;
        }

#if defined(__APPLE__)
        std::uint32_t requiredSize = 0;
        _NSGetExecutablePath(nullptr, &requiredSize);
        if (requiredSize == 0)
        {
            return false;
        }

        std::vector<char> pathBuffer(requiredSize + 1U, '\0');
        if (_NSGetExecutablePath(pathBuffer.data(), &requiredSize) != 0)
        {
            return false;
        }

        *outPath = std::filesystem::path(pathBuffer.data());
        return true;
#else
        char pathBuffer[PATH_MAX + 1] = {};
        const ssize_t length =
            readlink("/proc/self/exe", pathBuffer, PATH_MAX);
        if (length <= 0 || length > PATH_MAX)
        {
            return false;
        }

        pathBuffer[length] = '\0';
        *outPath = std::filesystem::path(pathBuffer);
        return true;
#endif
    }
}

bool LceGetExecutableDirectory(std::string* outDirectory)
{
    if (outDirectory == nullptr)
    {
        return false;
    }

    std::filesystem::path executablePath;
    if (!GetExecutablePath(&executablePath))
    {
        return false;
    }

    std::error_code errorCode;
    std::filesystem::path normalizedPath =
        std::filesystem::weakly_canonical(executablePath, errorCode);
    if (errorCode)
    {
        normalizedPath = executablePath;
    }

    const std::filesystem::path directory = normalizedPath.parent_path();
    if (directory.empty())
    {
        return false;
    }

    *outDirectory = directory.string();
    return true;
}

bool LceSetCurrentDirectoryToExecutable()
{
    std::string executableDirectory;
    if (!LceGetExecutableDirectory(&executableDirectory))
    {
        return false;
    }

    std::error_code errorCode;
    std::filesystem::current_path(executableDirectory, errorCode);
    return !errorCode;
}
