#include "lce_filesystem.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>

namespace
{
bool GetPathStatus(const char* path, struct stat* outStatus)
{
    if (path == nullptr || outStatus == nullptr)
    {
        return false;
    }

    return stat(path, outStatus) == 0;
}
}

bool FileOrDirectoryExists(const char* path)
{
    struct stat status;
    return GetPathStatus(path, &status);
}

bool FileExists(const char* path)
{
    struct stat status;
    return GetPathStatus(path, &status) && S_ISREG(status.st_mode);
}

bool DirectoryExists(const char* path)
{
    struct stat status;
    return GetPathStatus(path, &status) && S_ISDIR(status.st_mode);
}

bool CreateDirectoryIfMissing(const char* path, bool* outCreated)
{
    if (outCreated != nullptr)
    {
        *outCreated = false;
    }
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }

    struct stat status;
    if (GetPathStatus(path, &status))
    {
        return S_ISDIR(status.st_mode);
    }

    if (mkdir(path, 0755) == 0)
    {
        if (outCreated != nullptr)
        {
            *outCreated = true;
        }
        return true;
    }

    if (errno == EEXIST && GetPathStatus(path, &status))
    {
        return S_ISDIR(status.st_mode);
    }

    return false;
}

bool GetFirstFileInDirectory(const char* directory, char* outFilePath, size_t outFilePathSize)
{
    if (directory == nullptr || outFilePath == nullptr || outFilePathSize == 0)
    {
        return false;
    }

    DIR* handle = opendir(directory);
    if (handle == nullptr)
    {
        return false;
    }

    bool found = false;
    struct dirent* entry = nullptr;
    while ((entry = readdir(handle)) != nullptr)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        const int written = snprintf(
            outFilePath,
            outFilePathSize,
            "%s/%s",
            directory,
            entry->d_name);
        if (written < 0 || static_cast<size_t>(written) >= outFilePathSize)
        {
            continue;
        }

        if (FileExists(outFilePath))
        {
            found = true;
            break;
        }
    }

    closedir(handle);
    if (!found)
    {
        outFilePath[0] = '\0';
    }

    return found;
}
