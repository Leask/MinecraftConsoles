#include "lce_filesystem.h"

#ifdef _WINDOWS64
#include <windows.h>
#else
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <cstdio>

namespace
{
#ifndef _WINDOWS64
bool GetPathStatus(const char* path, struct stat* outStatus)
{
    if (path == nullptr || outStatus == nullptr)
    {
        return false;
    }

    return stat(path, outStatus) == 0;
}
#endif
}

bool FileOrDirectoryExists(const char* path)
{
#ifdef _WINDOWS64
    DWORD attribs = GetFileAttributesA(path);
    return (attribs != INVALID_FILE_ATTRIBUTES);
#else
    struct stat status;
    return GetPathStatus(path, &status);
#endif
}

bool FileExists(const char* path)
{
#ifdef _WINDOWS64
    DWORD attribs = GetFileAttributesA(path);
    return (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat status;
    return GetPathStatus(path, &status) && S_ISREG(status.st_mode);
#endif
}

bool DirectoryExists(const char* path)
{
#ifdef _WINDOWS64
    DWORD attribs = GetFileAttributesA(path);
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat status;
    return GetPathStatus(path, &status) && S_ISDIR(status.st_mode);
#endif
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

#ifdef _WINDOWS64
    DWORD attribs = GetFileAttributesA(path);
    if (attribs != INVALID_FILE_ATTRIBUTES)
    {
        return (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    if (CreateDirectoryA(path, nullptr))
    {
        if (outCreated != nullptr)
        {
            *outCreated = true;
        }
        return true;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        attribs = GetFileAttributesA(path);
        return attribs != INVALID_FILE_ATTRIBUTES &&
            (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    return false;
#else
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
#endif
}

bool GetFirstFileInDirectory(const char* directory, char* outFilePath, size_t outFilePathSize)
{
#ifdef _WINDOWS64
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", directory);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    do
    {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Found a file, copy its path to the output buffer
            snprintf(outFilePath, outFilePathSize, "%s\\%s", directory, findData.cFileName);
            FindClose(hFind);
            return true;
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
    return false; // No files found in the directory
#else
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
#endif
}
