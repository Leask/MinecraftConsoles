#include <cstdio>

#include "lce_filesystem/lce_filesystem.h"

int main(int argc, char* argv[])
{
    const char* path = argc > 1 ? argv[1] : ".";
    char firstFile[4096] = {};

    const bool exists = FileOrDirectoryExists(path);
    const bool isFile = FileExists(path);
    const bool isDirectory = DirectoryExists(path);
    const bool foundFirstFile = isDirectory &&
        GetFirstFileInDirectory(path, firstFile, sizeof(firstFile));

    printf("path=%s\n", path);
    printf("exists=%d file=%d directory=%d\n", exists, isFile, isDirectory);
    if (foundFirstFile)
    {
        printf("first_file=%s\n", firstFile);
    }
    else
    {
        printf("first_file=\n");
    }

    return exists ? 0 : 1;
}
