#include <cstdio>

#include "lce_filesystem/lce_filesystem.h"
#include "lce_net/lce_net.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"
#include "lce_win32/lce_win32.h"

namespace
{
    DWORD WINAPI SmokeThreadMain(LPVOID lpThreadParameter)
    {
        int* value = static_cast<int*>(lpThreadParameter);
        *value += 1;
        return 7;
    }
}

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
    CRITICAL_SECTION criticalSection = {};
    InitializeCriticalSection(&criticalSection);
    const ULONG recursiveEnter1 = TryEnterCriticalSection(&criticalSection);
    const ULONG recursiveEnter2 = TryEnterCriticalSection(&criticalSection);
    LeaveCriticalSection(&criticalSection);
    LeaveCriticalSection(&criticalSection);

    const DWORD tlsSlot = TlsAlloc();
    int tlsValue = 42;
    const BOOL tlsSet = TlsSetValue(tlsSlot, &tlsValue);
    const int tlsResolved = TlsGetValue(tlsSlot) == &tlsValue;

    HANDLE eventA = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    HANDLE eventB = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    SetEvent(eventB);
    const HANDLE anyHandles[] = {eventA, eventB};
    const DWORD waitAny = WaitForMultipleObjects(2, anyHandles, FALSE, 20);
    const DWORD waitAllBefore = WaitForMultipleObjects(2, anyHandles, TRUE, 0);
    SetEvent(eventA);
    const DWORD waitAllAfter = WaitForMultipleObjects(2, anyHandles, TRUE, 20);

    int threadValue = 0;
    DWORD createdThreadId = 0;
    HANDLE workerThread = CreateThread(
        nullptr,
        0,
        SmokeThreadMain,
        &threadValue,
        CREATE_SUSPENDED,
        &createdThreadId);
    const DWORD resumed = ResumeThread(workerThread);
    const DWORD threadWait = WaitForSingleObject(workerThread, 1000);
    DWORD threadExitCode = 0;
    const BOOL gotExitCode = GetExitCodeThread(workerThread, &threadExitCode);

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
    printf("critical_section_try=%lu recursive_try=%lu\n",
        static_cast<unsigned long>(recursiveEnter1),
        static_cast<unsigned long>(recursiveEnter2));
    printf("tls_slot=%lu tls_set=%d tls_resolved=%d\n",
        static_cast<unsigned long>(tlsSlot),
        tlsSet,
        tlsResolved);
    printf("wait_any=%lu wait_all_before=%lu wait_all_after=%lu\n",
        static_cast<unsigned long>(waitAny),
        static_cast<unsigned long>(waitAllBefore),
        static_cast<unsigned long>(waitAllAfter));
    printf("thread_id=%lu resumed=%lu thread_wait=%lu exit_code_ok=%d "
        "thread_exit_code=%lu thread_value=%d\n",
        static_cast<unsigned long>(createdThreadId),
        static_cast<unsigned long>(resumed),
        static_cast<unsigned long>(threadWait),
        gotExitCode,
        static_cast<unsigned long>(threadExitCode),
        threadValue);
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
    TlsFree(tlsSlot);
    CloseHandle(workerThread);
    CloseHandle(eventA);
    CloseHandle(eventB);
    DeleteCriticalSection(&criticalSection);

    return (exists && smokeDirectoryReady && netInitialized && ipv4Literal &&
        ipv6Literal && !invalidLiteral && recursiveEnter1 == TRUE &&
        recursiveEnter2 == TRUE && tlsSet == TRUE && tlsResolved == 1 &&
        waitAny == WAIT_OBJECT_0 + 1 && waitAllBefore == WAIT_TIMEOUT &&
        waitAllAfter == WAIT_OBJECT_0 && resumed == 0 &&
        threadWait == WAIT_OBJECT_0 && gotExitCode == TRUE &&
        threadExitCode == 7 && threadValue == 1 && deltaMs > 0 && deltaNs > 0)
        ? 0
        : 1;
}
