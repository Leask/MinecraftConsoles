#include <cstdio>

#include "lce_filesystem/lce_filesystem.h"
#include "lce_net/lce_net.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"

int main(int argc, char* argv[])
{
    const char* path = argc > 1 ? argv[1] : ".";
    char firstFile[4096] = {};
    bool createdDirectory = false;
    const bool smokeDirectoryReady = CreateDirectoryIfMissing(
        "build/portability-smoke-dir",
        &createdDirectory);
    const bool netInitialized = LceNetInitialize();
    const bool ipv4Literal = LceNetStringIsIpLiteral("127.0.0.1");
    const bool ipv6Literal = LceNetStringIsIpLiteral("::1");
    const bool invalidLiteral = LceNetStringIsIpLiteral("not-an-ip");
    const bool stdinAvailable = LceStdinIsAvailable();
    const int stdinReadableNow = stdinAvailable ? LceWaitForStdinReadable(0) : -1;
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
    printf("net_initialized=%d ipv4_literal=%d ipv6_literal=%d invalid_literal=%d\n",
        netInitialized,
        ipv4Literal,
        ipv6Literal,
        invalidLiteral);
    printf("stdin_available=%d stdin_readable_now=%d\n",
        stdinAvailable,
        stdinReadableNow);
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

    LceNetShutdown();

    return (exists && smokeDirectoryReady && netInitialized && ipv4Literal &&
        ipv6Literal && !invalidLiteral && deltaMs > 0 && deltaNs > 0)
        ? 0
        : 1;
}
