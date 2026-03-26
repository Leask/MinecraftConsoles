#include <cstdio>

#include "lce_filesystem/lce_filesystem.h"
#include "lce_time/lce_time.h"

int main(int argc, char* argv[])
{
    const char* path = argc > 1 ? argv[1] : ".";
    char firstFile[4096] = {};
    bool createdDirectory = false;
    const bool smokeDirectoryReady = CreateDirectoryIfMissing(
        "build/portability-smoke-dir",
        &createdDirectory);
    const std::uint64_t monotonicMsBefore = LceGetMonotonicMilliseconds();
    const std::uint64_t monotonicNsBefore = LceGetMonotonicNanoseconds();
    const std::uint64_t unixMs = LceGetUnixTimeMilliseconds();

    const bool exists = FileOrDirectoryExists(path);
    const bool isFile = FileExists(path);
    const bool isDirectory = DirectoryExists(path);
    const bool foundFirstFile = isDirectory &&
        GetFirstFileInDirectory(path, firstFile, sizeof(firstFile));

    LceSleepMilliseconds(20);

    const std::uint64_t monotonicMsAfter = LceGetMonotonicMilliseconds();
    const std::uint64_t monotonicNsAfter = LceGetMonotonicNanoseconds();
    const std::uint64_t deltaMs = monotonicMsAfter - monotonicMsBefore;
    const std::uint64_t deltaNs = monotonicNsAfter - monotonicNsBefore;

    printf("path=%s\n", path);
    printf("exists=%d file=%d directory=%d\n", exists, isFile, isDirectory);
    printf("smoke_directory_ready=%d created=%d\n",
        smokeDirectoryReady,
        createdDirectory);
    if (foundFirstFile)
    {
        printf("first_file=%s\n", firstFile);
    }
    else
    {
        printf("first_file=\n");
    }
    printf("unix_ms=%llu\n", static_cast<unsigned long long>(unixMs));
    printf("delta_ms=%llu delta_ns=%llu\n",
        static_cast<unsigned long long>(deltaMs),
        static_cast<unsigned long long>(deltaNs));

    return (exists && smokeDirectoryReady && deltaMs > 0 && deltaNs > 0)
        ? 0
        : 1;
}
