#include <cstdio>

#include "Minecraft.Server/Access/BanManager.h"
#include "lce_filesystem/lce_filesystem.h"
#include "lce_net/lce_net.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"
#include "lce_win32/lce_win32.h"
#include "Minecraft.Server/Common/FileUtils.h"
#include "Minecraft.Server/Console/IServerCliInputSink.h"
#include "Minecraft.Server/Console/ServerCliInput.h"
#include "Minecraft.Server/Common/StringUtils.h"
#include "Minecraft.Server/ServerLogger.h"
#include "Minecraft.Server/vendor/linenoise/linenoise.h"

namespace
{
    class SmokeCliInputSink : public ServerRuntime::IServerCliInputSink
    {
    public:
        void EnqueueCommandLine(const std::string &line) override
        {
            queuedLines.push_back(line);
        }

        void BuildCompletions(
            const std::string &line,
            std::vector<std::string> *out) const override
        {
            if (out == nullptr)
            {
                return;
            }

            out->clear();
            out->push_back("stop");
            out->push_back(line + "_status");
        }

        std::vector<std::string> queuedLines;
    };

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
    const std::wstring smokeWide = L"native \u4e16\u754c";
    const std::string smokeUtf8 = ServerRuntime::StringUtils::WideToUtf8(smokeWide);
    const std::wstring smokeRoundTrip =
        ServerRuntime::StringUtils::Utf8ToWide(smokeUtf8);
    unsigned long long parsedUnsigned = 0;
    const bool parsedUnsignedOk =
        ServerRuntime::StringUtils::TryParseUnsignedLongLong(
            " 12345 ",
            &parsedUnsigned);
    const std::string utcTimestamp =
        ServerRuntime::StringUtils::GetCurrentUtcTimestampIso8601();
    unsigned long long parsedUtcFileTime = 0;
    const bool parsedUtcOk =
        ServerRuntime::StringUtils::TryParseUtcTimestampIso8601(
            utcTimestamp,
            &parsedUtcFileTime);
    const std::string smokeFilePath = "build/portability-smoke-file.txt";
    const std::string smokeFileText = "native smoke file\n";
    const bool wroteSmokeFile =
        ServerRuntime::FileUtils::WriteTextFileAtomic(
            smokeFilePath,
            smokeFileText);
    std::string smokeFileReadback;
    const bool readSmokeFile =
        ServerRuntime::FileUtils::ReadTextFile(
            smokeFilePath,
            &smokeFileReadback);
    const unsigned long long utcFileTime =
        ServerRuntime::FileUtils::GetCurrentUtcFileTime();
    const std::string smokeBanDirectory = "build/portability-ban-smoke";
    const bool createdBanDirectory =
        CreateDirectoryIfMissing(smokeBanDirectory.c_str(), nullptr);
    ServerRuntime::Access::BanManager banManager(smokeBanDirectory);
    const bool ensuredBanFiles = banManager.EnsureBanFilesExist();
    ServerRuntime::Access::BannedPlayerEntry playerBan = {};
    playerBan.xuid = "12345";
    playerBan.name = "Smoke";
    playerBan.metadata =
        ServerRuntime::Access::BanManager::BuildDefaultMetadata("smoke");
    ServerRuntime::Access::BannedIpEntry ipBan = {};
    ipBan.ip = " 127.0.0.1 ";
    ipBan.metadata =
        ServerRuntime::Access::BanManager::BuildDefaultMetadata("smoke");
    const bool addedPlayerBan = banManager.AddPlayerBan(playerBan);
    const bool addedIpBan = banManager.AddIpBan(ipBan);
    const bool reloadedBans = banManager.Reload();
    const bool isPlayerBanned = banManager.IsPlayerBannedByXuid("12345");
    const bool isIpBanned = banManager.IsIpBanned("127.0.0.1");
    std::vector<ServerRuntime::Access::BannedPlayerEntry> snapshotPlayers;
    std::vector<ServerRuntime::Access::BannedIpEntry> snapshotIps;
    const bool snapshotPlayersOk =
        banManager.SnapshotBannedPlayers(&snapshotPlayers);
    const bool snapshotIpsOk =
        banManager.SnapshotBannedIps(&snapshotIps);
    ServerRuntime::EServerLogLevel parsedLogLevel = ServerRuntime::eServerLogLevel_Info;
    const bool parsedWarnLogLevel =
        ServerRuntime::TryParseServerLogLevel("Warn", &parsedLogLevel);
    ServerRuntime::SetServerLogLevel(ServerRuntime::eServerLogLevel_Warn);
    const int currentLogLevel = (int)ServerRuntime::GetServerLogLevel();
    linenoiseResetStop();
    const int historyMaxLenSet = linenoiseHistorySetMaxLen(8);
    const int historyAddFirst = linenoiseHistoryAdd("say native");
    const int historyAddSecond = linenoiseHistoryAdd("stop");
    linenoiseCompletions completions = {};
    linenoiseAddCompletion(&completions, "stop");
    linenoiseAddCompletion(&completions, "status");
    const bool completionEntriesOk = completions.len == 2 &&
        completions.cvec != nullptr &&
        completions.cvec[0] != nullptr &&
        completions.cvec[1] != nullptr;
    linenoiseExternalWriteBegin();
    linenoiseExternalWriteEnd();
    linenoiseRequestStop();
    linenoiseResetStop();
    SmokeCliInputSink cliSink;
    ServerRuntime::ServerCliInput cliInput;
    cliInput.Start(&cliSink);
    LceSleepMilliseconds(20);
    const bool cliRunning = cliInput.IsRunning();
    cliInput.Stop();
    const bool cliStopped = !cliInput.IsRunning();
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
    printf("string_utf8_size=%zu string_round_trip=%d parsed_unsigned_ok=%d "
        "parsed_unsigned=%llu utc_timestamp=%s parsed_utc_ok=%d\n",
        smokeUtf8.size(),
        smokeRoundTrip == smokeWide,
        parsedUnsignedOk,
        parsedUnsigned,
        utcTimestamp.c_str(),
        parsedUtcOk);
    printf("file_write=%d file_read=%d file_readback_match=%d utc_file_time=%llu\n",
        wroteSmokeFile,
        readSmokeFile,
        smokeFileReadback == smokeFileText,
        utcFileTime);
    printf("ban_dir=%d ban_files=%d add_player=%d add_ip=%d reload=%d "
        "player_banned=%d ip_banned=%d snapshot_players=%d snapshot_ips=%d "
        "snapshot_player_count=%zu snapshot_ip_count=%zu\n",
        createdBanDirectory,
        ensuredBanFiles,
        addedPlayerBan,
        addedIpBan,
        reloadedBans,
        isPlayerBanned,
        isIpBanned,
        snapshotPlayersOk,
        snapshotIpsOk,
        snapshotPlayers.size(),
        snapshotIps.size());
    printf("log_parse=%d log_level=%d\n",
        parsedWarnLogLevel,
        currentLogLevel);
    printf("linenoise_history=%d,%d,%d completion_entries=%d\n",
        historyMaxLenSet,
        historyAddFirst,
        historyAddSecond,
        completionEntriesOk);
    printf("cli_running=%d cli_stopped=%d queued_lines=%zu\n",
        cliRunning,
        cliStopped,
        cliSink.queuedLines.size());
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
    if (completions.cvec != nullptr)
    {
        for (size_t i = 0; i < completions.len; ++i)
        {
            linenoiseFree(completions.cvec[i]);
        }
        linenoiseFree(completions.cvec);
    }

    return (exists && smokeDirectoryReady && netInitialized && ipv4Literal &&
        ipv6Literal && !invalidLiteral && recursiveEnter1 == TRUE &&
        recursiveEnter2 == TRUE && tlsSet == TRUE && tlsResolved == 1 &&
        !smokeUtf8.empty() && smokeRoundTrip == smokeWide &&
        parsedUnsignedOk && parsedUnsigned == 12345ULL &&
        parsedUtcOk && parsedUtcFileTime > 0 && parsedUtcFileTime <= utcFileTime &&
        wroteSmokeFile && readSmokeFile && smokeFileReadback == smokeFileText &&
        utcFileTime > 0 &&
        createdBanDirectory && ensuredBanFiles &&
        addedPlayerBan && addedIpBan && reloadedBans &&
        isPlayerBanned && isIpBanned &&
        snapshotPlayersOk && snapshotIpsOk &&
        snapshotPlayers.size() == 1 && snapshotIps.size() == 1 &&
        parsedWarnLogLevel &&
        parsedLogLevel == ServerRuntime::eServerLogLevel_Warn &&
        currentLogLevel == (int)ServerRuntime::eServerLogLevel_Warn &&
        historyMaxLenSet == 1 && historyAddFirst == 1 &&
        historyAddSecond == 1 && completionEntriesOk &&
        cliRunning && cliStopped && cliSink.queuedLines.empty() &&
        utcTimestamp.size() == 20 && utcTimestamp[10] == 'T' &&
        utcTimestamp[19] == 'Z' &&
        waitAny == WAIT_OBJECT_0 + 1 && waitAllBefore == WAIT_TIMEOUT &&
        waitAllAfter == WAIT_OBJECT_0 && resumed == 0 &&
        threadWait == WAIT_OBJECT_0 && gotExitCode == TRUE &&
        threadExitCode == 7 && threadValue == 1 && deltaMs > 0 && deltaNs > 0)
        ? 0
        : 1;
}
