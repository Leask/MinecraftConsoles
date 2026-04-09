#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#if defined(_WINDOWS64) || defined(_WIN32)
#include <stdlib.h>
#else
#include <cstdlib>
#endif

#include "Minecraft.Client/Common/App_Defines.h"
#include "Minecraft.Server/Access/Access.h"
#include "Minecraft.Server/Access/BanManager.h"
#include "Minecraft.Server/Common/DedicatedServerAutosaveTracker.h"
#include "Minecraft.Server/Common/DedicatedServerBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerGameplayLoop.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/DedicatedServerLifecycle.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "Minecraft.Server/Common/NativeDedicatedServerLoadedSaveState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "NativeDedicatedServerHostedGameCore.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformState.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSessionConfig.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/DedicatedServerSocketBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerWorldBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerWorldLoadPipeline.h"
#include "Minecraft.Server/Common/DedicatedServerWorldLoadStorage.h"
#include "Minecraft.Server/Common/DedicatedServerWorldSaveSelection.h"
#include "Minecraft.Server/Common/DedicatedServerShutdownPlan.h"
#include "lce_filesystem/lce_filesystem.h"
#include "lce_net/lce_net.h"
#include "lce_process/lce_process.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"
#include "lce_win32/lce_win32.h"
#include "Minecraft.Server/Common/DedicatedServerOptions.h"
#include "Minecraft.Server/Common/DedicatedServerRuntime.h"
#include "Minecraft.Server/Common/FileUtils.h"
#include "Minecraft.Server/Common/ServerStoragePaths.h"
#include "Minecraft.Server/Console/IServerCliInputSink.h"
#include "Minecraft.Server/Console/ServerCliInput.h"
#include "Minecraft.Server/Common/StringUtils.h"
#include "Minecraft.Server/ServerLogger.h"
#include "Minecraft.Server/WorldManager.h"
#include "Minecraft.Server/vendor/linenoise/linenoise.h"
#include "Minecraft.World/PerformanceTimer.h"
#include "ServerPropertiesSmoke.h"
#include "WorldCompressionSmoke.h"
#include "Minecraft.World/ThreadName.h"
#include "WorldFileHeaderSmoke.h"
#include "WorldSystemSmoke.h"

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameSessionSnapshot
    StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
        const NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs = 0);

    bool IsNativeDedicatedServerHostedGameSessionRunning();

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    void ResetNativeDedicatedServerHostedGameSessionState();

    void StopNativeDedicatedServerHostedGameSession(
        std::uint64_t stoppedMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionActivity(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle);

    void ObserveNativeDedicatedServerHostedGameSessionContext(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &savePath,
        const std::string &storageRoot,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
        std::uint64_t sessionStartMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions);

    void ObserveNativeDedicatedServerHostedGameSessionActivation(
        int localUsersMask,
        bool onlineGame,
        bool privateGame,
        unsigned int publicSlots,
        unsigned int privateSlots,
        bool fakeLocalPlayerJoined);

    void ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionPlatformState(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount);

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid);

    void ObserveNativeDedicatedServerHostedGameSessionWorkerState(
        std::uint64_t pendingWorldActionTicks,
        std::uint64_t pendingAutosaveCommands,
        std::uint64_t pendingSaveCommands,
        std::uint64_t pendingStopCommands,
        std::uint64_t pendingHaltCommands,
        std::uint64_t workerTickCount,
        std::uint64_t completedWorkerActions,
        std::uint64_t processedAutosaveCommands,
        std::uint64_t processedSaveCommands,
        std::uint64_t processedStopCommands,
        std::uint64_t processedHaltCommands,
        std::uint64_t lastQueuedCommandId,
        std::uint64_t activeCommandId,
        std::uint64_t activeCommandTicksRemaining,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            activeCommandKind,
        std::uint64_t lastProcessedCommandId,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            lastProcessedCommandKind);

    void ObserveNativeDedicatedServerHostedGameSessionSummary(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay);

    void RequestNativeDedicatedServerHostedGameSessionAutosave(
        unsigned int workTicks,
        std::uint64_t nowMs = 0);

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);

    void ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
        std::uint64_t nowMs = 0);
}

namespace
{
    struct SmokeNetworkGameInitData;
    LoadSaveDataThreadParam *g_expectedHostedGameSaveData = nullptr;

    class ScopedCurrentDirectory
    {
    public:
        explicit ScopedCurrentDirectory(const std::filesystem::path& path)
            : m_previous(std::filesystem::current_path())
            , m_active(false)
        {
            std::error_code errorCode;
            std::filesystem::remove_all(path, errorCode);
            errorCode.clear();
            std::filesystem::create_directories(path, errorCode);
            std::filesystem::current_path(path, errorCode);
            m_active = !errorCode;
        }

        ~ScopedCurrentDirectory()
        {
            if (!m_active)
            {
                return;
            }

            std::error_code errorCode;
            std::filesystem::current_path(m_previous, errorCode);
        }

        bool IsActive() const
        {
            return m_active;
        }

    private:
        std::filesystem::path m_previous;
        bool m_active;
    };

    class ScopedEnvVar
    {
    public:
        ScopedEnvVar(const char *name, const char *value)
            : m_name(name != nullptr ? name : "")
            , m_hadPrevious(false)
        {
            if (m_name.empty())
            {
                return;
            }

            const char *previous = std::getenv(m_name.c_str());
            if (previous != nullptr)
            {
                m_hadPrevious = true;
                m_previous = previous;
            }

            Set(value != nullptr ? value : "");
        }

        ~ScopedEnvVar()
        {
            if (m_name.empty())
            {
                return;
            }

            if (m_hadPrevious)
            {
                Set(m_previous.c_str());
            }
            else
            {
                Unset();
            }
        }

    private:
        void Set(const char *value)
        {
#if defined(_WINDOWS64) || defined(_WIN32)
            _putenv_s(m_name.c_str(), value);
#else
            setenv(m_name.c_str(), value, 1);
#endif
        }

        void Unset()
        {
#if defined(_WINDOWS64) || defined(_WIN32)
            _putenv_s(m_name.c_str(), "");
#else
            unsetenv(m_name.c_str());
#endif
        }

        std::string m_name;
        std::string m_previous;
        bool m_hadPrevious;
    };

    int HostedGameRuntimeSmokeThreadProc(void* threadParam)
    {
        int* value = static_cast<int*>(threadParam);
        if (value != nullptr)
        {
            *value = 1;
        }
        return 11;
    }

    int HostedGameRuntimeSleepSmokeThreadProc(void* threadParam)
    {
        int* value = static_cast<int*>(threadParam);
        LceSleepMilliseconds(30);
        if (value != nullptr)
        {
            *value = 2;
        }
        return 13;
    }

    struct SmokeNetworkGameInitData
    {
        std::int64_t seed = 0;
        LoadSaveDataThreadParam *saveData = nullptr;
        DWORD settings = 0;
        bool dedicatedNoLocalHostPlayer = false;
        unsigned int xzSize = 0;
        unsigned char hellScale = 0;
    };

    int HostedGameStartupSmokeThreadProc(void* threadParam)
    {
        SmokeNetworkGameInitData* initData =
            static_cast<SmokeNetworkGameInitData*>(threadParam);
        if (initData == nullptr)
        {
            return -1;
        }

        return initData->seed == 4444 &&
                initData->saveData == g_expectedHostedGameSaveData &&
                initData->settings != 0 &&
                initData->dedicatedNoLocalHostPlayer &&
                initData->xzSize == 320U &&
                initData->hellScale == 8U
            ? 0
            : -2;
    }

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

    struct GameplayLoopRunPollContext
    {
        int pollCount = 0;
        int shutdownOnPoll = 3;
    };

    struct WaitHookSmokeContext
    {
        int tickCount = 0;
        int handleCount = 0;
        int idleAfterTicks = 0;
        bool halted = false;
    };

    struct WorldStorageHookSmokeContext
    {
        std::wstring appliedWorldName;
        std::string appliedSaveId;
        int resetCount = 0;
    };

    struct WorldLoadPipelineSmokeContext
    {
        std::vector<ServerRuntime::DedicatedServerWorldLoadCandidate>
            candidates;
        std::vector<unsigned char> bytes;
        int beginQueryCount = 0;
        int beginLoadCount = 0;
        int matchedIndex = -1;
        bool queryOk = true;
        bool loadOk = true;
        bool isCorrupt = false;
        bool readOk = true;
    };

    struct NativeHostedCoreHookSmokeContext
    {
        int readyCount = 0;
        int stoppedCount = 0;
        bool requestShutdownOnReady = false;
    };

    void GameplayLoopRunPoll(void* context)
    {
        GameplayLoopRunPollContext* pollContext =
            static_cast<GameplayLoopRunPollContext*>(context);
        if (pollContext == nullptr)
        {
            return;
        }

        ++pollContext->pollCount;
        if (pollContext->pollCount >= pollContext->shutdownOnPoll)
        {
            ServerRuntime::RequestDedicatedServerShutdown();
        }
    }

    void IncrementHookCount(void* context)
    {
        int* hookCount = static_cast<int*>(context);
        if (hookCount != nullptr)
        {
            ++(*hookCount);
        }
    }

    NativeHostedCoreHookSmokeContext g_nativeHostedCoreHookSmokeContext = {};

    void NativeHostedCoreReadyHook(std::uint64_t)
    {
        ++g_nativeHostedCoreHookSmokeContext.readyCount;
        if (g_nativeHostedCoreHookSmokeContext.requestShutdownOnReady)
        {
            ServerRuntime::RequestDedicatedServerShutdown();
        }
    }

    void NativeHostedCoreStoppedHook(std::uint64_t, std::uint64_t)
    {
        ++g_nativeHostedCoreHookSmokeContext.stoppedCount;
    }

    bool WaitHookIdleProc(int, void* context)
    {
        WaitHookSmokeContext* waitContext =
            static_cast<WaitHookSmokeContext*>(context);
        return waitContext != nullptr &&
            waitContext->tickCount >= waitContext->idleAfterTicks;
    }

    bool WaitHookHaltProc(void* context)
    {
        WaitHookSmokeContext* waitContext =
            static_cast<WaitHookSmokeContext*>(context);
        return waitContext != nullptr && waitContext->halted;
    }

    void WaitHookTickProc(void* context)
    {
        WaitHookSmokeContext* waitContext =
            static_cast<WaitHookSmokeContext*>(context);
        if (waitContext != nullptr)
        {
            ++waitContext->tickCount;
        }
    }

    void WaitHookHandleProc(void* context)
    {
        WaitHookSmokeContext* waitContext =
            static_cast<WaitHookSmokeContext*>(context);
        if (waitContext != nullptr)
        {
            ++waitContext->handleCount;
        }
    }

    void ApplyWorldStorageNameHook(
        const std::wstring& worldName,
        void* context)
    {
        WorldStorageHookSmokeContext* storageContext =
            static_cast<WorldStorageHookSmokeContext*>(context);
        if (storageContext != nullptr)
        {
            storageContext->appliedWorldName = worldName;
        }
    }

    void ApplyWorldStorageSaveIdHook(
        const std::string& saveId,
        void* context)
    {
        WorldStorageHookSmokeContext* storageContext =
            static_cast<WorldStorageHookSmokeContext*>(context);
        if (storageContext != nullptr)
        {
            storageContext->appliedSaveId = saveId;
        }
    }

    void ResetWorldStorageHook(void* context)
    {
        WorldStorageHookSmokeContext* storageContext =
            static_cast<WorldStorageHookSmokeContext*>(context);
        if (storageContext != nullptr)
        {
            ++storageContext->resetCount;
        }
    }

    bool ClearWorldLoadPipelineHook(void*)
    {
        return true;
    }

    bool BeginWorldLoadPipelineQueryHook(int, void* context)
    {
        WorldLoadPipelineSmokeContext* loadContext =
            static_cast<WorldLoadPipelineSmokeContext*>(context);
        if (loadContext == nullptr)
        {
            return false;
        }

        ++loadContext->beginQueryCount;
        return loadContext->queryOk;
    }

    bool PollWorldLoadPipelineQueryHook(
        DWORD,
        ServerRuntime::DedicatedServerWorldLoadTickProc,
        void* context,
        std::vector<ServerRuntime::DedicatedServerWorldLoadCandidate>*
            outCandidates)
    {
        WorldLoadPipelineSmokeContext* loadContext =
            static_cast<WorldLoadPipelineSmokeContext*>(context);
        if (loadContext == nullptr ||
            outCandidates == nullptr ||
            !loadContext->queryOk)
        {
            return false;
        }

        *outCandidates = loadContext->candidates;
        return true;
    }

    bool BeginWorldLoadPipelineLoadHook(int matchedIndex, void* context)
    {
        WorldLoadPipelineSmokeContext* loadContext =
            static_cast<WorldLoadPipelineSmokeContext*>(context);
        if (loadContext == nullptr || !loadContext->loadOk)
        {
            return false;
        }

        ++loadContext->beginLoadCount;
        loadContext->matchedIndex = matchedIndex;
        return true;
    }

    bool PollWorldLoadPipelineLoadHook(
        DWORD,
        ServerRuntime::DedicatedServerWorldLoadTickProc,
        void* context,
        bool* outIsCorrupt)
    {
        WorldLoadPipelineSmokeContext* loadContext =
            static_cast<WorldLoadPipelineSmokeContext*>(context);
        if (loadContext == nullptr || !loadContext->loadOk)
        {
            return false;
        }

        if (outIsCorrupt != nullptr)
        {
            *outIsCorrupt = loadContext->isCorrupt;
        }
        return true;
    }

    bool ReadWorldLoadPipelineBytesHook(
        void* context,
        std::vector<unsigned char>* outBytes)
    {
        WorldLoadPipelineSmokeContext* loadContext =
            static_cast<WorldLoadPipelineSmokeContext*>(context);
        if (loadContext == nullptr ||
            outBytes == nullptr ||
            !loadContext->readOk)
        {
            return false;
        }

        *outBytes = loadContext->bytes;
        return true;
    }

    DWORD WINAPI SmokeThreadMain(LPVOID lpThreadParameter)
    {
        SetThreadName(static_cast<DWORD>(-1), "smoke-worker");
        int* value = static_cast<int*>(lpThreadParameter);
        *value += 1;
        return 7;
    }
}

int main(int argc, char* argv[])
{
    const char* path = argc > 1 ? argv[1] : ".";
    std::error_code cleanupError;
    std::filesystem::remove_all(
        "build/portability-smoke-dir",
        cleanupError);
    cleanupError.clear();
    std::filesystem::remove(
        "build/portability-smoke-file.txt",
        cleanupError);
    cleanupError.clear();
    std::filesystem::remove_all(
        "build/portability-ban-smoke",
        cleanupError);
    char firstFile[4096] = {};
    bool createdDirectory = false;
    const bool smokeDirectoryReady = CreateDirectoryIfMissing(
        "build/portability-smoke-dir",
        &createdDirectory);
    std::string executableDirectory;
    const bool resolvedExecutableDirectory =
        LceGetExecutableDirectory(&executableDirectory);
    const bool netInitialized = LceNetInitialize();
    const bool ipv4Literal = LceNetStringIsIpLiteral("127.0.0.1");
    const bool ipv6Literal = LceNetStringIsIpLiteral("::1");
    const bool invalidLiteral = LceNetStringIsIpLiteral("not-an-ip");
    const LceSocketHandle udpReceiver = LceNetOpenUdpSocket();
    const bool udpReceiverReuse =
        LceNetSetSocketReuseAddress(udpReceiver, true);
    const bool udpReceiverTimeout =
        LceNetSetSocketRecvTimeout(udpReceiver, 1000);
    const bool udpReceiverBound =
        LceNetBindIpv4(udpReceiver, "127.0.0.1", 0);
    const int udpReceiverPort = LceNetGetBoundPort(udpReceiver);
    const LceSocketHandle udpSender = LceNetOpenUdpSocket();
    const bool udpSenderBroadcast =
        LceNetSetSocketBroadcast(udpSender, true);
    const char udpPayload[] = "udp-smoke";
    const int udpSent = LceNetSendToIpv4(
        udpSender,
        "127.0.0.1",
        udpReceiverPort,
        udpPayload,
        static_cast<int>(sizeof(udpPayload) - 1));
    char udpRecvBuf[64] = {};
    char udpSenderIp[64] = {};
    int udpSenderPort = 0;
    const int udpRecv = LceNetRecvFromIpv4(
        udpReceiver,
        udpRecvBuf,
        sizeof(udpRecvBuf),
        udpSenderIp,
        sizeof(udpSenderIp),
        &udpSenderPort);
    const bool udpPayloadOk = udpRecv == static_cast<int>(sizeof(udpPayload) - 1) &&
        std::memcmp(udpRecvBuf, udpPayload, sizeof(udpPayload) - 1) == 0;
    const LceSocketHandle tcpListener = LceNetOpenTcpSocket();
    const bool tcpListenerReuse =
        LceNetSetSocketReuseAddress(tcpListener, true);
    const bool tcpListenerBound =
        LceNetBindIpv4(tcpListener, "127.0.0.1", 0);
    const int tcpListenerPort = LceNetGetBoundPort(tcpListener);
    char resolvedLocalhost[64] = {};
    const bool tcpResolvedLocalhost = LceNetResolveIpv4(
        "localhost",
        tcpListenerPort,
        resolvedLocalhost,
        sizeof(resolvedLocalhost));
    const bool tcpListening = LceNetListen(tcpListener, 4);
    const LceSocketHandle tcpClient = LceNetOpenTcpSocket();
    const bool tcpClientNoDelay =
        LceNetSetSocketNoDelay(tcpClient, true);
    const bool tcpClientTimeout =
        LceNetSetSocketRecvTimeout(tcpClient, 1000);
    const bool tcpConnected = LceNetConnectIpv4(
        tcpClient,
        "127.0.0.1",
        tcpListenerPort);
    char tcpAcceptedIp[64] = {};
    int tcpAcceptedPort = 0;
    const LceSocketHandle tcpAccepted = LceNetAcceptIpv4(
        tcpListener,
        tcpAcceptedIp,
        sizeof(tcpAcceptedIp),
        &tcpAcceptedPort);
    const bool tcpAcceptedNoDelay =
        LceNetSetSocketNoDelay(tcpAccepted, true);
    const bool tcpAcceptedTimeout =
        LceNetSetSocketRecvTimeout(tcpAccepted, 1000);
    const char tcpClientPayload[] = "tcp-client";
    const bool tcpClientSent = LceNetSendAll(
        tcpClient,
        tcpClientPayload,
        static_cast<int>(sizeof(tcpClientPayload) - 1));
    char tcpServerBuf[64] = {};
    const bool tcpServerReceived = LceNetRecvAll(
        tcpAccepted,
        tcpServerBuf,
        static_cast<int>(sizeof(tcpClientPayload) - 1));
    const bool tcpServerPayloadOk =
        std::memcmp(
            tcpServerBuf,
            tcpClientPayload,
            sizeof(tcpClientPayload) - 1) == 0;
    const char tcpServerPayload[] = "tcp-server";
    const bool tcpServerSent = LceNetSendAll(
        tcpAccepted,
        tcpServerPayload,
        static_cast<int>(sizeof(tcpServerPayload) - 1));
    char tcpClientBuf[64] = {};
    const bool tcpClientReceived = LceNetRecvAll(
        tcpClient,
        tcpClientBuf,
        static_cast<int>(sizeof(tcpServerPayload) - 1));
    const bool tcpClientPayloadOk =
        std::memcmp(
            tcpClientBuf,
            tcpServerPayload,
            sizeof(tcpServerPayload) - 1) == 0;
    const bool lanWireSizeOk = sizeof(LceLanBroadcast) == 84;
    LceLanBroadcast lanBroadcast = {};
    const bool lanBroadcastBuilt = LceLanBuildBroadcast(
        25565,
        L"SmokeHost",
        0x1234,
        77,
        3,
        45,
        2,
        256,
        true,
        &lanBroadcast);
    LceLanSession lanSession = {};
    const bool lanBroadcastDecoded = LceLanDecodeBroadcast(
        &lanBroadcast,
        sizeof(lanBroadcast),
        "192.168.0.7",
        1000,
        &lanSession);
    std::vector<LceLanSession> lanSessions;
    bool lanAdded = false;
    LceLanUpsertSession(lanSession, &lanSessions, &lanAdded);
    LceLanSession lanUpdated = lanSession;
    lanUpdated.playerCount = 4;
    lanUpdated.isJoinable = false;
    lanUpdated.lastSeenMs = 2000;
    bool lanUpdatedAdded = true;
    LceLanUpsertSession(lanUpdated, &lanSessions, &lanUpdatedAdded);
    std::vector<LceLanSession> expiredLanSessions;
    LceLanPruneExpiredSessions(8000, 5000, &lanSessions, &expiredLanSessions);
    const bool stdinAvailable = LceStdinIsAvailable();
    const int stdinReadableNow = stdinAvailable ? LceWaitForStdinReadable(0) : -1;
    SetThreadName(GetCurrentThreadId(), "smoke-main");
    PerformanceTimer timer;
    const std::uint64_t monotonicMsBefore = LceGetMonotonicMilliseconds();
    const std::uint64_t monotonicNsBefore = LceGetMonotonicNanoseconds();
    const std::uint64_t unixMs = LceGetUnixTimeMilliseconds();
    const WorldFileHeaderSmokeResult worldFileHeaderSmoke =
        RunWorldFileHeaderSmoke();
    const ServerPropertiesSmokeResult serverPropertiesSmoke =
        RunServerPropertiesSmoke();
    const WorldCompressionSmokeResult worldCompressionSmoke =
        RunWorldCompressionSmoke();
    const WorldSystemSmokeResult worldSystemSmoke = RunWorldSystemSmoke();
    const std::filesystem::path smokeWorkingDirectory =
        std::filesystem::current_path();
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
    const ServerRuntime::DedicatedServerWorldSaveMatchEntry
        worldSaveEntries[] = {
            {L"Alpha World", L"save_alpha"},
            {L"Beta World", L"WORLD_B"}
        };
    const ServerRuntime::DedicatedServerWorldSaveSelectionResult
        worldSaveMatchById =
            ServerRuntime::SelectDedicatedServerWorldSaveEntry(
                worldSaveEntries,
                2,
                L"alpha world",
                "world_b");
    const ServerRuntime::DedicatedServerWorldSaveSelectionResult
        worldSaveMatchByName =
            ServerRuntime::SelectDedicatedServerWorldSaveEntry(
                worldSaveEntries,
                2,
                L"beta world",
                "");
    const ServerRuntime::DedicatedServerWorldSaveSelectionResult
        worldSaveNoMatch =
            ServerRuntime::SelectDedicatedServerWorldSaveEntry(
                worldSaveEntries,
                2,
                L"gamma world",
                "missing");
    const ServerRuntime::DedicatedServerConfig defaultDedicatedConfig =
        ServerRuntime::CreateDefaultDedicatedServerConfig();
    ServerRuntime::ServerPropertiesConfig dedicatedProperties = {};
    dedicatedProperties.serverPort = 25570;
    dedicatedProperties.serverIp = "10.0.0.5";
    dedicatedProperties.serverName = "NativeProps";
    dedicatedProperties.maxPlayers = 24;
    dedicatedProperties.worldSize = 2;
    dedicatedProperties.worldSizeChunks = 72;
    dedicatedProperties.worldHellScale = 2;
    dedicatedProperties.logLevel = ServerRuntime::eServerLogLevel_Warn;
    dedicatedProperties.hasSeed = true;
    dedicatedProperties.seed = 987654321LL;
    ServerRuntime::DedicatedServerConfig propertyDedicatedConfig =
        defaultDedicatedConfig;
    ServerRuntime::ApplyServerPropertiesToDedicatedConfig(
        dedicatedProperties,
        &propertyDedicatedConfig);
    char dedicatedArg0[] = "Minecraft.Server";
    char dedicatedArg1[] = "-port";
    char dedicatedArg2[] = "25571";
    char dedicatedArg3[] = "-bind";
    char dedicatedArg4[] = "127.0.0.1";
    char dedicatedArg5[] = "-name";
    char dedicatedArg6[] = "NativeCli";
    char dedicatedArg7[] = "-maxplayers";
    char dedicatedArg8[] = "32";
    char dedicatedArg9[] = "-seed";
    char dedicatedArg10[] = "123456789";
    char dedicatedArg11[] = "-loglevel";
    char dedicatedArg12[] = "debug";
    char* dedicatedArgv[] = {
        dedicatedArg0,
        dedicatedArg1,
        dedicatedArg2,
        dedicatedArg3,
        dedicatedArg4,
        dedicatedArg5,
        dedicatedArg6,
        dedicatedArg7,
        dedicatedArg8,
        dedicatedArg9,
        dedicatedArg10,
        dedicatedArg11,
        dedicatedArg12
    };
    ServerRuntime::DedicatedServerConfig cliDedicatedConfig =
        propertyDedicatedConfig;
    std::string dedicatedParseError;
    const bool dedicatedParseOk =
        ServerRuntime::ParseDedicatedServerCommandLine(
            static_cast<int>(sizeof(dedicatedArgv) / sizeof(dedicatedArgv[0])),
            dedicatedArgv,
            &cliDedicatedConfig,
            &dedicatedParseError);
    char dedicatedHelpArg0[] = "Minecraft.Server";
    char dedicatedHelpArg1[] = "--help";
    char* dedicatedHelpArgv[] = {
        dedicatedHelpArg0,
        dedicatedHelpArg1
    };
    ServerRuntime::DedicatedServerConfig helpDedicatedConfig =
        defaultDedicatedConfig;
    std::string dedicatedHelpError;
    const bool dedicatedHelpOk =
        ServerRuntime::ParseDedicatedServerCommandLine(
            static_cast<int>(
                sizeof(dedicatedHelpArgv) / sizeof(dedicatedHelpArgv[0])),
            dedicatedHelpArgv,
            &helpDedicatedConfig,
            &dedicatedHelpError);
    char dedicatedEphemeralArg0[] = "Minecraft.Server";
    char dedicatedEphemeralArg1[] = "-port";
    char dedicatedEphemeralArg2[] = "0";
    char* dedicatedEphemeralArgv[] = {
        dedicatedEphemeralArg0,
        dedicatedEphemeralArg1,
        dedicatedEphemeralArg2
    };
    ServerRuntime::DedicatedServerConfig ephemeralDedicatedConfig =
        defaultDedicatedConfig;
    std::string dedicatedEphemeralError;
    const bool dedicatedEphemeralOk =
        ServerRuntime::ParseDedicatedServerCommandLine(
            static_cast<int>(
                sizeof(dedicatedEphemeralArgv) /
                sizeof(dedicatedEphemeralArgv[0])),
            dedicatedEphemeralArgv,
            &ephemeralDedicatedConfig,
            &dedicatedEphemeralError);
    char dedicatedInvalidArg0[] = "Minecraft.Server";
    char dedicatedInvalidArg1[] = "-port";
    char dedicatedInvalidArg2[] = "70000";
    char* dedicatedInvalidArgv[] = {
        dedicatedInvalidArg0,
        dedicatedInvalidArg1,
        dedicatedInvalidArg2
    };
    ServerRuntime::DedicatedServerConfig invalidDedicatedConfig =
        defaultDedicatedConfig;
    std::string dedicatedInvalidError;
    const bool dedicatedInvalidOk =
        ServerRuntime::ParseDedicatedServerCommandLine(
            static_cast<int>(
                sizeof(dedicatedInvalidArgv) /
                sizeof(dedicatedInvalidArgv[0])),
            dedicatedInvalidArgv,
            &invalidDedicatedConfig,
            &dedicatedInvalidError);
    std::vector<std::string> dedicatedUsageLines;
    ServerRuntime::BuildDedicatedServerUsageLines(&dedicatedUsageLines);
    ServerRuntime::DedicatedServerWorldLoadStorageRuntime
        nativeWorldLoadStorageRuntime =
            ServerRuntime::CreateDedicatedServerPlatformWorldLoadStorageRuntime();
    const bool nativeWorldLoadStorageRuntimeReady =
        nativeWorldLoadStorageRuntime.storageHooks.setWorldTitleProc !=
            nullptr &&
        nativeWorldLoadStorageRuntime.storageHooks.setWorldTitleContext !=
            nullptr &&
        nativeWorldLoadStorageRuntime.storageHooks.setWorldSaveIdProc !=
            nullptr &&
        nativeWorldLoadStorageRuntime.storageHooks.setWorldSaveIdContext !=
            nullptr &&
        nativeWorldLoadStorageRuntime.storageHooks.resetSaveDataProc !=
            nullptr &&
        nativeWorldLoadStorageRuntime.storageHooks.resetSaveDataContext !=
            nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.clearSavesProc != nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.beginQueryProc != nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.pollQueryProc != nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.beginLoadProc != nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.pollLoadProc != nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.readBytesProc != nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.context != nullptr &&
        nativeWorldLoadStorageRuntime.platformContext != nullptr &&
        nativeWorldLoadStorageRuntime.destroyContextProc != nullptr;
    ServerRuntime::DestroyDedicatedServerWorldLoadStorageRuntime(
        &nativeWorldLoadStorageRuntime);
    const bool nativeWorldLoadStorageRuntimeDestroyed =
        nativeWorldLoadStorageRuntime.storageHooks.setWorldTitleProc ==
            nullptr &&
        nativeWorldLoadStorageRuntime.storageHooks.setWorldSaveIdProc ==
            nullptr &&
        nativeWorldLoadStorageRuntime.storageHooks.resetSaveDataProc ==
            nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.clearSavesProc == nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.beginQueryProc == nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.pollQueryProc == nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.beginLoadProc == nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.pollLoadProc == nullptr &&
        nativeWorldLoadStorageRuntime.loadHooks.readBytesProc == nullptr &&
        nativeWorldLoadStorageRuntime.platformContext == nullptr &&
        nativeWorldLoadStorageRuntime.destroyContextProc == nullptr;
    ServerRuntime::ServerPropertiesConfig sessionProperties = {};
    sessionProperties.difficulty = 2;
    sessionProperties.gameMode = 1;
    sessionProperties.friendsOfFriends = true;
    sessionProperties.gamertags = true;
    sessionProperties.bedrockFog = true;
    sessionProperties.levelTypeFlat = true;
    sessionProperties.generateStructures = false;
    sessionProperties.bonusChest = true;
    sessionProperties.pvp = true;
    sessionProperties.trustPlayers = false;
    sessionProperties.fireSpreads = false;
    sessionProperties.tnt = true;
    sessionProperties.hostCanFly = true;
    sessionProperties.hostCanChangeHunger = false;
    sessionProperties.hostCanBeInvisible = true;
    sessionProperties.disableSaving = true;
    sessionProperties.mobGriefing = false;
    sessionProperties.keepInventory = true;
    sessionProperties.doMobSpawning = true;
    sessionProperties.doMobLoot = false;
    sessionProperties.doTileDrops = true;
    sessionProperties.naturalRegeneration = false;
    sessionProperties.doDaylightCycle = true;
    ServerRuntime::ServerPropertiesConfig defaultWorldTargetProperties = {};
    ServerRuntime::ServerPropertiesConfig configuredWorldTargetProperties = {};
    configuredWorldTargetProperties.worldName = L"Smoke World";
    configuredWorldTargetProperties.worldSaveId = "SMOKE_WORLD";
    const ServerRuntime::DedicatedServerWorldTarget defaultWorldTarget =
        ServerRuntime::ResolveDedicatedServerWorldTarget(
            defaultWorldTargetProperties);
    const ServerRuntime::DedicatedServerWorldTarget configuredWorldTarget =
        ServerRuntime::ResolveDedicatedServerWorldTarget(
            configuredWorldTargetProperties);
    const ServerRuntime::DedicatedServerWorldStoragePlan
        configuredWorldStoragePlan =
            ServerRuntime::BuildConfiguredDedicatedServerWorldStoragePlan(
                configuredWorldTarget);
    const ServerRuntime::DedicatedServerWorldStoragePlan
        loadedWorldStoragePlan =
            ServerRuntime::BuildLoadedDedicatedServerWorldStoragePlan(
                configuredWorldTarget,
                "resolved_world");
    const ServerRuntime::DedicatedServerWorldStoragePlan
        fallbackLoadedWorldStoragePlan =
            ServerRuntime::BuildLoadedDedicatedServerWorldStoragePlan(
                configuredWorldTarget,
                "");
    const ServerRuntime::DedicatedServerWorldStoragePlan
        createdWorldStoragePlan =
            ServerRuntime::BuildCreatedDedicatedServerWorldStoragePlan(
                configuredWorldTarget);
    WorldStorageHookSmokeContext worldStorageHookContext = {};
    ServerRuntime::DedicatedServerWorldStorageHooks worldStorageHooks = {};
    worldStorageHooks.setWorldTitleProc = &ApplyWorldStorageNameHook;
    worldStorageHooks.setWorldTitleContext = &worldStorageHookContext;
    worldStorageHooks.setWorldSaveIdProc = &ApplyWorldStorageSaveIdHook;
    worldStorageHooks.setWorldSaveIdContext = &worldStorageHookContext;
    worldStorageHooks.resetSaveDataProc = &ResetWorldStorageHook;
    worldStorageHooks.resetSaveDataContext = &worldStorageHookContext;
    const bool appliedConfiguredWorldStorage =
        ServerRuntime::ApplyDedicatedServerWorldStoragePlan(
            configuredWorldStoragePlan,
            worldStorageHooks);
    const bool appliedLoadedWorldStorage =
        ServerRuntime::ApplyDedicatedServerWorldStoragePlan(
            loadedWorldStoragePlan,
            worldStorageHooks);
    const bool appliedCreatedWorldStorage =
        ServerRuntime::ApplyDedicatedServerWorldStoragePlan(
            createdWorldStoragePlan,
            worldStorageHooks);
    WorldLoadPipelineSmokeContext worldLoadPipelineContext = {};
    worldLoadPipelineContext.candidates.push_back(
        {L"Alpha World", L"alpha_save"});
    worldLoadPipelineContext.candidates.push_back(
        {L"Smoke World", L"resolved_world"});
    worldLoadPipelineContext.bytes = {1, 2, 3, 4};
    ServerRuntime::DedicatedServerWorldLoadHooks worldLoadPipelineHooks = {};
    worldLoadPipelineHooks.clearSavesProc = &ClearWorldLoadPipelineHook;
    worldLoadPipelineHooks.beginQueryProc = &BeginWorldLoadPipelineQueryHook;
    worldLoadPipelineHooks.pollQueryProc = &PollWorldLoadPipelineQueryHook;
    worldLoadPipelineHooks.beginLoadProc = &BeginWorldLoadPipelineLoadHook;
    worldLoadPipelineHooks.pollLoadProc = &PollWorldLoadPipelineLoadHook;
    worldLoadPipelineHooks.readBytesProc = &ReadWorldLoadPipelineBytesHook;
    worldLoadPipelineHooks.context = &worldLoadPipelineContext;
    ServerRuntime::DedicatedServerWorldLoadPayload worldLoadPayload = {};
    const ServerRuntime::EDedicatedServerWorldLoadStatus
        worldLoadPipelineStatus =
            ServerRuntime::LoadDedicatedServerWorldData(
                configuredWorldTarget,
                0,
                nullptr,
                worldLoadPipelineHooks,
                &worldLoadPayload);
    WorldLoadPipelineSmokeContext worldLoadPipelineNotFoundContext = {};
    worldLoadPipelineNotFoundContext.candidates.push_back(
        {L"Alpha World", L"alpha_save"});
    ServerRuntime::DedicatedServerWorldLoadHooks
        worldLoadPipelineNotFoundHooks = worldLoadPipelineHooks;
    worldLoadPipelineNotFoundHooks.context = &worldLoadPipelineNotFoundContext;
    ServerRuntime::DedicatedServerWorldLoadPayload
        worldLoadPipelineNotFoundPayload = {};
    const ServerRuntime::EDedicatedServerWorldLoadStatus
        worldLoadPipelineNotFoundStatus =
            ServerRuntime::LoadDedicatedServerWorldData(
                configuredWorldTarget,
                0,
                nullptr,
                worldLoadPipelineNotFoundHooks,
                &worldLoadPipelineNotFoundPayload);
    WorldLoadPipelineSmokeContext worldLoadPipelineCorruptContext = {};
    worldLoadPipelineCorruptContext.candidates =
        worldLoadPipelineContext.candidates;
    worldLoadPipelineCorruptContext.bytes =
        worldLoadPipelineContext.bytes;
    worldLoadPipelineCorruptContext.isCorrupt = true;
    ServerRuntime::DedicatedServerWorldLoadHooks
        worldLoadPipelineCorruptHooks = worldLoadPipelineHooks;
    worldLoadPipelineCorruptHooks.context = &worldLoadPipelineCorruptContext;
    ServerRuntime::DedicatedServerWorldLoadPayload
        worldLoadPipelineCorruptPayload = {};
    const ServerRuntime::EDedicatedServerWorldLoadStatus
        worldLoadPipelineCorruptStatus =
            ServerRuntime::LoadDedicatedServerWorldData(
                configuredWorldTarget,
                0,
                nullptr,
                worldLoadPipelineCorruptHooks,
                &worldLoadPipelineCorruptPayload);
    LoadSaveDataThreadParam *ownedLoadSaveData =
        ServerRuntime::CreateOwnedLoadSaveDataThreadParam(
            worldLoadPayload.bytes,
            worldLoadPayload.matchedTitle);
    bool ownedLoadSaveDataCopied = false;
    if (ownedLoadSaveData != nullptr)
    {
        worldLoadPayload.bytes[0] = 9;
        const unsigned char *ownedBytes =
            static_cast<const unsigned char *>(ownedLoadSaveData->data);
        ownedLoadSaveDataCopied =
            ownedLoadSaveData->fileSize == 4 &&
            ownedLoadSaveData->saveName == L"Smoke World" &&
            ownedBytes != nullptr &&
            ownedBytes[0] == 1;
    }
    ServerRuntime::DestroyLoadSaveDataThreadParam(ownedLoadSaveData);
    LoadSaveDataThreadParam *fakeBootstrapSaveData =
        reinterpret_cast<LoadSaveDataThreadParam *>(0x4321);
    const ServerRuntime::WorldBootstrapResult loadedWorldTargetBootstrap =
        ServerRuntime::BuildDedicatedServerWorldBootstrapResult(
            ServerRuntime::eDedicatedServerWorldLoad_Loaded,
            fakeBootstrapSaveData,
            configuredWorldTarget,
            "resolved_world");
    const ServerRuntime::WorldBootstrapResult createdWorldTargetBootstrap =
        ServerRuntime::BuildDedicatedServerWorldBootstrapResult(
            ServerRuntime::eDedicatedServerWorldLoad_NotFound,
            fakeBootstrapSaveData,
            configuredWorldTarget,
            "");
    const ServerRuntime::WorldBootstrapResult failedWorldTargetBootstrap =
        ServerRuntime::BuildDedicatedServerWorldBootstrapResult(
            ServerRuntime::eDedicatedServerWorldLoad_Failed,
            fakeBootstrapSaveData,
            configuredWorldTarget,
            "");
    ServerRuntime::DedicatedServerConfig sessionDedicatedConfig =
        defaultDedicatedConfig;
    sessionDedicatedConfig.maxPlayers = 256;
    sessionDedicatedConfig.hasSeed = true;
    sessionDedicatedConfig.seed = 4444;
    sessionDedicatedConfig.worldSize = 4;
    sessionDedicatedConfig.worldSizeChunks = 320;
    sessionDedicatedConfig.worldHellScale = 8;
    const ServerRuntime::DedicatedServerSessionConfig sessionConfig =
        ServerRuntime::BuildDedicatedServerSessionConfig(
            sessionDedicatedConfig,
            sessionProperties);
    const ServerRuntime::DedicatedServerAppSessionPlan appSessionPlan =
        ServerRuntime::BuildDedicatedServerAppSessionPlan(sessionConfig);
    ServerRuntime::DedicatedServerAppSessionApplyResult
        appSessionRuntimeResult = {};
    ServerRuntime::ServerPropertiesConfig worldBootstrapProperties = {};
    worldBootstrapProperties.worldName = L"";
    worldBootstrapProperties.worldSaveId = "WORLD";
    ServerRuntime::WorldBootstrapResult loadedWorldBootstrap;
    loadedWorldBootstrap.status = ServerRuntime::eWorldBootstrap_Loaded;
    loadedWorldBootstrap.resolvedSaveId = "world_loaded";
    const ServerRuntime::DedicatedServerWorldBootstrapPlan loadedWorldPlan =
        ServerRuntime::BuildDedicatedServerWorldBootstrapPlan(
            worldBootstrapProperties,
            loadedWorldBootstrap);
    const ServerRuntime::DedicatedServerWorldLoadPlan loadedWorldLoadPlan =
        ServerRuntime::BuildDedicatedServerWorldLoadPlan(
            loadedWorldPlan);
    ServerRuntime::DedicatedServerWorldLoadExecutionResult
        loadedWorldExecution = {};
    ServerRuntime::WorldBootstrapResult createdWorldBootstrap;
    createdWorldBootstrap.status = ServerRuntime::eWorldBootstrap_CreatedNew;
    createdWorldBootstrap.resolvedSaveId = "world";
    const ServerRuntime::DedicatedServerWorldBootstrapPlan createdWorldPlan =
        ServerRuntime::BuildDedicatedServerWorldBootstrapPlan(
            worldBootstrapProperties,
            createdWorldBootstrap);
    const ServerRuntime::DedicatedServerInitialSavePlan createdInitialSavePlan =
        ServerRuntime::BuildDedicatedServerInitialSavePlan(
            createdWorldPlan,
            false,
            false);
    ServerRuntime::WorldBootstrapResult failedWorldBootstrap;
    failedWorldBootstrap.status = ServerRuntime::eWorldBootstrap_Failed;
    const ServerRuntime::DedicatedServerWorldBootstrapPlan failedWorldPlan =
        ServerRuntime::BuildDedicatedServerWorldBootstrapPlan(
            worldBootstrapProperties,
            failedWorldBootstrap);
    const ServerRuntime::DedicatedServerWorldLoadPlan failedWorldLoadPlan =
        ServerRuntime::BuildDedicatedServerWorldLoadPlan(
            failedWorldPlan);
    ServerRuntime::DedicatedServerWorldLoadExecutionResult
        failedWorldExecution = {};
    std::string worldLoadExecutionText;
    bool worldLoadExecutionDirectoryReady = false;
    {
        ScopedCurrentDirectory directoryGuard(
            "build/portability-world-load-smoke");
        worldLoadExecutionDirectoryReady = directoryGuard.IsActive();
        if (worldLoadExecutionDirectoryReady)
        {
            ServerRuntime::ServerPropertiesConfig loadedWorldExecutionConfig =
                worldBootstrapProperties;
            loadedWorldExecutionConfig.worldName = L"World Load Smoke";
            loadedWorldExecution =
                ServerRuntime::ExecuteDedicatedServerWorldLoadPlan(
                    loadedWorldPlan,
                    loadedWorldLoadPlan,
                    &loadedWorldExecutionConfig);
            ServerRuntime::FileUtils::ReadTextFile(
                "server.properties",
                &worldLoadExecutionText);

            ServerRuntime::ServerPropertiesConfig failedWorldExecutionConfig =
                worldBootstrapProperties;
            failedWorldExecution =
                ServerRuntime::ExecuteDedicatedServerWorldLoadPlan(
                    failedWorldPlan,
                    failedWorldLoadPlan,
                    &failedWorldExecutionConfig);
        }
    }
    ServerRuntime::WorldBootstrapResult nativeWorldManagerBootstrap = {};
    bool nativeWorldManagerDirectoryReady = false;
    bool nativeWorldManagerStorageRootReady = false;
    {
        ScopedCurrentDirectory directoryGuard(
            "build/portability-world-manager-smoke");
        nativeWorldManagerDirectoryReady = directoryGuard.IsActive();
        if (nativeWorldManagerDirectoryReady)
        {
            ServerRuntime::ServerPropertiesConfig nativeWorldManagerConfig =
                defaultWorldTargetProperties;
            nativeWorldManagerConfig.worldName = L"Native Smoke World";
            nativeWorldManagerConfig.worldSaveId = "NATIVE_SMOKE_WORLD";
            nativeWorldManagerBootstrap =
                ServerRuntime::BootstrapWorldForServer(
                    nativeWorldManagerConfig,
                    0,
                    nullptr);
            nativeWorldManagerStorageRootReady =
                std::filesystem::exists("NativeDesktop/GameHDD");
        }
    }
    ServerRuntime::WorldBootstrapResult nativeLoadedWorldManagerBootstrap = {};
    bool nativeLoadedWorldManagerDirectoryReady = false;
    bool nativeLoadedWorldManagerStorageRootReady = false;
    bool nativeLoadedWorldManagerPayloadCopied = false;
    ServerRuntime::WorldBootstrapResult
        nativeCorruptWorldManagerBootstrap = {};
    bool nativeCorruptWorldManagerDirectoryReady = false;
    bool nativeCorruptWorldManagerStorageRootReady = false;
    {
        ScopedCurrentDirectory directoryGuard(
            "build/portability-world-manager-loaded-smoke");
        nativeLoadedWorldManagerDirectoryReady = directoryGuard.IsActive();
        if (nativeLoadedWorldManagerDirectoryReady)
        {
            std::filesystem::create_directories("NativeDesktop/GameHDD");
            ServerRuntime::NativeDedicatedServerSaveStub
                loadedWorldSaveStub = {};
            loadedWorldSaveStub.worldName = "Native Loaded World";
            loadedWorldSaveStub.levelId = "NATIVE_LOADED_WORLD";
            loadedWorldSaveStub.startupMode = "loaded";
            loadedWorldSaveStub.sessionPhase = "running";
            loadedWorldSaveStub.payloadName = "NATIVE_LOADED_WORLD";
            loadedWorldSaveStub.payloadBytes = 128;
            loadedWorldSaveStub.payloadChecksum = 0x1020304050607080ULL;
            loadedWorldSaveStub.sessionActive = true;
            loadedWorldSaveStub.worldActionIdle = true;
            loadedWorldSaveStub.startupPayloadPresent = true;
            loadedWorldSaveStub.startupPayloadValidated = true;
            std::string loadedWorldSaveText;
            const bool loadedWorldSaveTextBuilt =
                ServerRuntime::BuildNativeDedicatedServerSaveStubText(
                    loadedWorldSaveStub,
                    &loadedWorldSaveText);
            std::FILE *loadedWorldFile = std::fopen(
                "NativeDesktop/GameHDD/NATIVE_LOADED_WORLD.save",
                "wb");
            if (loadedWorldFile != nullptr && loadedWorldSaveTextBuilt)
            {
                std::fwrite(
                    loadedWorldSaveText.data(),
                    sizeof(loadedWorldSaveText[0]),
                    loadedWorldSaveText.size(),
                    loadedWorldFile);
                std::fclose(loadedWorldFile);
            }
            else if (loadedWorldFile != nullptr)
            {
                std::fclose(loadedWorldFile);
            }

            ServerRuntime::ServerPropertiesConfig
                nativeLoadedWorldManagerConfig = defaultWorldTargetProperties;
            nativeLoadedWorldManagerConfig.worldName =
                L"Native Loaded World";
            nativeLoadedWorldManagerConfig.worldSaveId =
                "NATIVE_LOADED_WORLD";
            nativeLoadedWorldManagerBootstrap =
                ServerRuntime::BootstrapWorldForServer(
                    nativeLoadedWorldManagerConfig,
                    0,
                    nullptr);
            nativeLoadedWorldManagerStorageRootReady =
                std::filesystem::exists("NativeDesktop/GameHDD");
            if (nativeLoadedWorldManagerBootstrap.saveData != nullptr)
            {
                const unsigned char *payloadBytes =
                    static_cast<const unsigned char *>(
                        nativeLoadedWorldManagerBootstrap.saveData->data);
                const std::vector<unsigned char> loadedPayloadBytes(
                    loadedWorldSaveText.begin(),
                    loadedWorldSaveText.end());
                nativeLoadedWorldManagerPayloadCopied =
                    nativeLoadedWorldManagerBootstrap.saveData->fileSize ==
                        (int)loadedPayloadBytes.size() &&
                    payloadBytes != nullptr &&
                    std::equal(
                        loadedPayloadBytes.begin(),
                        loadedPayloadBytes.end(),
                        payloadBytes) &&
                    nativeLoadedWorldManagerBootstrap.saveData->saveName ==
                        L"Native Loaded World";
                ServerRuntime::DestroyLoadSaveDataThreadParam(
                    nativeLoadedWorldManagerBootstrap.saveData);
                nativeLoadedWorldManagerBootstrap.saveData = nullptr;
            }
        }
    }
    {
        ScopedCurrentDirectory directoryGuard(
            "build/portability-world-manager-corrupt-smoke");
        nativeCorruptWorldManagerDirectoryReady = directoryGuard.IsActive();
        if (nativeCorruptWorldManagerDirectoryReady)
        {
            std::filesystem::create_directories("NativeDesktop/GameHDD");
            std::FILE *corruptWorldFile = std::fopen(
                "NativeDesktop/GameHDD/NATIVE_CORRUPT_WORLD.save",
                "wb");
            if (corruptWorldFile != nullptr)
            {
                const char corruptWorldBytes[] = "not-a-native-save-stub";
                std::fwrite(
                    corruptWorldBytes,
                    sizeof(corruptWorldBytes[0]),
                    sizeof(corruptWorldBytes) - 1,
                    corruptWorldFile);
                std::fclose(corruptWorldFile);
            }

            ServerRuntime::ServerPropertiesConfig
                nativeCorruptWorldManagerConfig = defaultWorldTargetProperties;
            nativeCorruptWorldManagerConfig.worldName =
                L"Native Corrupt World";
            nativeCorruptWorldManagerConfig.worldSaveId =
                "NATIVE_CORRUPT_WORLD";
            nativeCorruptWorldManagerBootstrap =
                ServerRuntime::BootstrapWorldForServer(
                    nativeCorruptWorldManagerConfig,
                    0,
                    nullptr);
            nativeCorruptWorldManagerStorageRootReady =
                std::filesystem::exists("NativeDesktop/GameHDD");
        }
    }
    const bool shouldRunInitialSave =
        ServerRuntime::ShouldRunDedicatedServerInitialSave(
            createdWorldPlan,
            false,
            false);
    const bool shouldSkipInitialSaveWhenStopping =
        !ServerRuntime::ShouldRunDedicatedServerInitialSave(
            createdWorldPlan,
            true,
            false);
    const std::vector<unsigned char> fakeSaveBytes = { 7, 8, 9, 10 };
    const std::uint64_t fakeSaveChecksum = 0xb2697a66c2589349ULL;
    LoadSaveDataThreadParam *fakeSaveData =
        ServerRuntime::CreateOwnedLoadSaveDataThreadParam(
            fakeSaveBytes,
            L"Smoke World");
    ServerRuntime::NativeDedicatedServerSaveStub nativeHostedSaveStub = {};
    nativeHostedSaveStub.worldName = "world";
    nativeHostedSaveStub.levelId = "world";
    nativeHostedSaveStub.startupMode = "loaded";
    nativeHostedSaveStub.sessionPhase = "stopped";
    nativeHostedSaveStub.payloadName = "world";
    nativeHostedSaveStub.sessionCompleted = true;
    nativeHostedSaveStub.worldActionIdle = true;
    nativeHostedSaveStub.onlineGame = true;
    nativeHostedSaveStub.privateGame = false;
    nativeHostedSaveStub.fakeLocalPlayerJoined = true;
    nativeHostedSaveStub.publicSlots = 16;
    nativeHostedSaveStub.privateSlots = 1;
    nativeHostedSaveStub.startupPayloadPresent = true;
    nativeHostedSaveStub.startupPayloadValidated = true;
    nativeHostedSaveStub.startupThreadIterations = 4;
    nativeHostedSaveStub.startupThreadDurationMs = 20;
    std::string nativeHostedSaveText;
    const bool nativeHostedSaveTextBuilt =
        ServerRuntime::BuildNativeDedicatedServerSaveStubText(
            nativeHostedSaveStub,
            &nativeHostedSaveText);
    const std::vector<unsigned char> nativeHostedSaveBytes(
        nativeHostedSaveText.begin(),
        nativeHostedSaveText.end());
    LoadSaveDataThreadParam *nativeHostedSaveData =
        nativeHostedSaveTextBuilt
            ? ServerRuntime::CreateOwnedLoadSaveDataThreadParam(
                nativeHostedSaveBytes,
                L"world")
            : nullptr;
    g_expectedHostedGameSaveData = fakeSaveData;
    loadedWorldBootstrap.saveData = fakeSaveData;
    createdWorldBootstrap.saveData = fakeSaveData;
    failedWorldBootstrap.saveData = fakeSaveData;
    const ServerRuntime::DedicatedServerNetworkInitPlan networkInitPlan =
        ServerRuntime::BuildDedicatedServerNetworkInitPlan(
            sessionConfig,
            fakeSaveData,
            7777);
    const std::int64_t resolvedConfiguredSeed =
        ServerRuntime::ResolveDedicatedServerSeed(
            sessionConfig,
            9999);
    ServerRuntime::DedicatedServerConfig randomSeedDedicatedConfig =
        sessionDedicatedConfig;
    randomSeedDedicatedConfig.hasSeed = false;
    const ServerRuntime::DedicatedServerSessionConfig randomSeedSessionConfig =
        ServerRuntime::BuildDedicatedServerSessionConfig(
            randomSeedDedicatedConfig,
            sessionProperties);
    const std::int64_t resolvedGeneratedSeed =
        ServerRuntime::ResolveDedicatedServerSeed(
            randomSeedSessionConfig,
            9999);
    const ServerRuntime::DedicatedServerHostedGamePlan hostedGamePlan =
        ServerRuntime::BuildDedicatedServerHostedGamePlan(
            sessionConfig,
            fakeSaveData,
            9999);
    SmokeNetworkGameInitData populatedInitData;
    ServerRuntime::PopulateDedicatedServerNetworkGameInitData(
        &populatedInitData,
        hostedGamePlan.networkInitPlan);
    const ServerRuntime::DedicatedServerShutdownPlan shutdownPlanWithServer =
        ServerRuntime::BuildDedicatedServerShutdownPlan(
            true,
            true);
    const ServerRuntime::DedicatedServerShutdownPlan shutdownPlanWithoutServer =
        ServerRuntime::BuildDedicatedServerShutdownPlan(
            false,
            false);
    const ServerRuntime::DedicatedServerHostedThreadStartupPlan
        threadStartupPlanOk =
            ServerRuntime::BuildDedicatedServerHostedThreadStartupPlan(0);
    const ServerRuntime::DedicatedServerHostedThreadStartupPlan
        threadStartupPlanFailed =
            ServerRuntime::BuildDedicatedServerHostedThreadStartupPlan(9);
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    const bool shutdownSignalInitiallyClear =
        !ServerRuntime::IsDedicatedServerShutdownRequested();
    ServerRuntime::RequestDedicatedServerShutdown();
    const bool shutdownSignalRaised =
        ServerRuntime::IsDedicatedServerShutdownRequested();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    const bool shutdownSignalReset =
        !ServerRuntime::IsDedicatedServerShutdownRequested();
    ServerRuntime::ResetDedicatedServerAutosaveTracker();
    ServerRuntime::MarkDedicatedServerAutosaveTrackerRequested();
    ServerRuntime::MarkDedicatedServerAutosaveTrackerRequested();
    ServerRuntime::UpdateDedicatedServerAutosaveTracker(false);
    const ServerRuntime::DedicatedServerAutosaveTrackerSnapshot
        autosaveTrackerPendingSnapshot =
            ServerRuntime::GetDedicatedServerAutosaveTrackerSnapshot();
    ServerRuntime::UpdateDedicatedServerAutosaveTracker(true);
    ServerRuntime::UpdateDedicatedServerAutosaveTracker(true);
    const ServerRuntime::DedicatedServerAutosaveTrackerSnapshot
        autosaveTrackerCompletedSnapshot =
            ServerRuntime::GetDedicatedServerAutosaveTrackerSnapshot();
    const std::string smokeFilePath = "build/portability-smoke-file.txt";
    const std::string smokeFileText = "native smoke file\n";
    ServerRuntime::ServerPropertiesConfig runtimeProperties = {};
    runtimeProperties.lanAdvertise = true;
    runtimeProperties.autosaveIntervalSeconds = 45;
    ServerRuntime::DedicatedServerConfig runtimeConfig =
        defaultDedicatedConfig;
    std::snprintf(
        runtimeConfig.bindIP,
        sizeof(runtimeConfig.bindIP),
        "%s",
        "192.168.0.8");
    std::snprintf(
        runtimeConfig.name,
        sizeof(runtimeConfig.name),
        "%s",
        "RuntimeHost");
    runtimeConfig.port = 25590;
    const ServerRuntime::DedicatedServerRuntimeState runtimeState =
        ServerRuntime::BuildDedicatedServerRuntimeState(
            runtimeConfig,
            runtimeProperties);
    const ServerRuntime::DedicatedServerPlatformState platformState =
        ServerRuntime::BuildDedicatedServerPlatformState(runtimeState, 3);
    const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
        platformRuntimeResult =
            ServerRuntime::StartDedicatedServerPlatformRuntime(platformState);
    appSessionRuntimeResult =
        ServerRuntime::ApplyDedicatedServerAppSessionPlan(appSessionPlan);
    SmokeNetworkGameInitData startupExecutionInitData = {};
    ServerRuntime::DedicatedServerStartupExecutionResult
        startupExecutionResult = {};
    ServerRuntime::DedicatedServerStartupExecutionResult
        startupExecutionFailed = {};
    std::string startupExecutionText;
    bool startupExecutionDirectoryReady = false;
    {
        ScopedCurrentDirectory directoryGuard(
            "build/portability-startup-execution-smoke");
        startupExecutionDirectoryReady = directoryGuard.IsActive();
        if (startupExecutionDirectoryReady)
        {
            ServerRuntime::ServerPropertiesConfig startupExecutionProperties =
                worldBootstrapProperties;
            startupExecutionProperties.worldName = L"Startup Execution";
            startupExecutionResult =
                ServerRuntime::ExecuteDedicatedServerStartup(
                    sessionDedicatedConfig,
                    &startupExecutionProperties,
                    loadedWorldBootstrap,
                    9999,
                    &HostedGameStartupSmokeThreadProc,
                    &startupExecutionInitData);
            ServerRuntime::FileUtils::ReadTextFile(
                "server.properties",
                &startupExecutionText);

            ServerRuntime::ServerPropertiesConfig failedStartupProperties =
                worldBootstrapProperties;
            startupExecutionFailed =
                ServerRuntime::ExecuteDedicatedServerStartup(
                    sessionDedicatedConfig,
                    &failedStartupProperties,
                    failedWorldBootstrap,
                    9999,
                    &HostedGameStartupSmokeThreadProc,
                    &startupExecutionInitData);
        }
    }
    ServerRuntime::TickDedicatedServerPlatformRuntime();
    ServerRuntime::HandleDedicatedServerPlatformActions();
    const bool platformActionIdle =
        ServerRuntime::IsDedicatedServerWorldActionIdle(0);
    ServerRuntime::RequestDedicatedServerWorldAutosave(0);
    const bool platformActionPending =
        !ServerRuntime::IsDedicatedServerWorldActionIdle(0);
    const bool platformWaitStillBusy =
        !ServerRuntime::WaitForDedicatedServerWorldActionIdle(0, 1);
    const bool platformWaitIdle =
        ServerRuntime::WaitForDedicatedServerWorldActionIdle(0, 100);
    const std::uint64_t platformAutosaveRequestsAfterWait =
        ServerRuntime::GetDedicatedServerAutosaveRequestCount();
    const std::uint64_t platformAutosaveCompletionsAfterWait =
        ServerRuntime::GetDedicatedServerAutosaveCompletionCount();
    WaitHookSmokeContext waitHookIdleContext = {};
    waitHookIdleContext.idleAfterTicks = 2;
    ServerRuntime::DedicatedServerWorldActionWaitHooks worldActionWaitHooks =
        {};
    worldActionWaitHooks.isIdleProc = &WaitHookIdleProc;
    worldActionWaitHooks.idleContext = &waitHookIdleContext;
    worldActionWaitHooks.haltProc = &WaitHookHaltProc;
    worldActionWaitHooks.haltContext = &waitHookIdleContext;
    worldActionWaitHooks.tickProc = &WaitHookTickProc;
    worldActionWaitHooks.tickContext = &waitHookIdleContext;
    worldActionWaitHooks.handleActionsProc = &WaitHookHandleProc;
    worldActionWaitHooks.handleActionsContext = &waitHookIdleContext;
    const bool worldActionWaitIdleOk =
        ServerRuntime::WaitForDedicatedServerWorldActionIdleWithHooks(
            0,
            100,
            worldActionWaitHooks);
    WaitHookSmokeContext waitHookTimeoutContext = {};
    waitHookTimeoutContext.idleAfterTicks = 1000;
    ServerRuntime::DedicatedServerWorldActionWaitHooks
        worldActionTimeoutHooks = worldActionWaitHooks;
    worldActionTimeoutHooks.idleContext = &waitHookTimeoutContext;
    worldActionTimeoutHooks.haltContext = &waitHookTimeoutContext;
    worldActionTimeoutHooks.tickContext = &waitHookTimeoutContext;
    worldActionTimeoutHooks.handleActionsContext = &waitHookTimeoutContext;
    const bool worldActionWaitTimedOut =
        !ServerRuntime::WaitForDedicatedServerWorldActionIdleWithHooks(
            0,
            0,
            worldActionTimeoutHooks);
    WaitHookSmokeContext waitHookHaltContext = {};
    waitHookHaltContext.halted = true;
    waitHookHaltContext.idleAfterTicks = 1000;
    ServerRuntime::DedicatedServerWorldActionWaitHooks
        worldActionHaltHooks = worldActionWaitHooks;
    worldActionHaltHooks.idleContext = &waitHookHaltContext;
    worldActionHaltHooks.haltContext = &waitHookHaltContext;
    worldActionHaltHooks.tickContext = &waitHookHaltContext;
    worldActionHaltHooks.handleActionsContext = &waitHookHaltContext;
    const bool worldActionWaitHalted =
        !ServerRuntime::WaitForDedicatedServerWorldActionIdleWithHooks(
            0,
            100,
            worldActionHaltHooks);
    const bool platformGameplayInstance =
        ServerRuntime::HasDedicatedServerGameplayInstance();
    const bool platformAppShutdownBefore =
        !ServerRuntime::IsDedicatedServerAppShutdownRequested();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(true);
    const bool platformAppShutdownAfter =
        ServerRuntime::IsDedicatedServerAppShutdownRequested();
    ServerRuntime::TickDedicatedServerPlatformRuntime();
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        platformFlagsSessionSnapshot =
            ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
    const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        platformFlagsRuntimeSnapshot =
            ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
    const bool platformFlagsProjected =
        platformFlagsSessionSnapshot.appShutdownRequested &&
        !platformFlagsSessionSnapshot.gameplayHalted &&
        platformFlagsSessionSnapshot.stopSignalValid &&
        platformFlagsRuntimeSnapshot.appShutdownRequested &&
        !platformFlagsRuntimeSnapshot.gameplayHalted &&
        platformFlagsRuntimeSnapshot.stopSignalValid;
    const bool platformGameplayHaltedBefore =
        !ServerRuntime::IsDedicatedServerGameplayHalted();
    const bool platformStopSignalValid =
        ServerRuntime::IsDedicatedServerStopSignalValid();
    ServerRuntime::EnableDedicatedServerSaveOnExit();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    const ServerRuntime::DedicatedServerInitialSaveExecutionResult
        initialSaveExecution =
            ServerRuntime::ExecuteDedicatedServerInitialSave(
                createdWorldPlan,
                0);
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    ServerRuntime::DedicatedServerAutosaveState gameplayLoopState =
        ServerRuntime::CreateDedicatedServerAutosaveState(
            0,
            runtimeProperties);
    gameplayLoopState.nextTickMs = 0;
    const ServerRuntime::DedicatedServerGameplayLoopIterationResult
        gameplayLoopRequest =
            ServerRuntime::TickDedicatedServerGameplayLoop(
                &gameplayLoopState,
                runtimeProperties,
                0,
                nullptr,
                nullptr);
    const ServerRuntime::DedicatedServerGameplayLoopIterationResult
        gameplayLoopPending =
            ServerRuntime::TickDedicatedServerGameplayLoop(
                &gameplayLoopState,
                runtimeProperties,
                0,
                nullptr,
                nullptr);
    const ServerRuntime::DedicatedServerGameplayLoopIterationResult
        gameplayLoopComplete =
            ServerRuntime::TickDedicatedServerGameplayLoop(
                &gameplayLoopState,
                runtimeProperties,
                0,
                nullptr,
                nullptr);
    ServerRuntime::RequestDedicatedServerShutdown();
    const ServerRuntime::DedicatedServerGameplayLoopIterationResult
        gameplayLoopExit =
            ServerRuntime::TickDedicatedServerGameplayLoop(
                &gameplayLoopState,
                runtimeProperties,
                0,
                nullptr,
                nullptr);
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    ServerRuntime::DedicatedServerAutosaveState gameplayLoopRunState =
        ServerRuntime::CreateDedicatedServerAutosaveState(
            0,
            runtimeProperties);
    gameplayLoopRunState.nextTickMs = 0;
    GameplayLoopRunPollContext gameplayLoopRunPollContext = {};
    const ServerRuntime::DedicatedServerGameplayLoopRunResult
        gameplayLoopRunResult =
            ServerRuntime::RunDedicatedServerGameplayLoopUntilExit(
                &gameplayLoopRunState,
                runtimeProperties,
                0,
                &GameplayLoopRunPoll,
                &gameplayLoopRunPollContext,
                0);
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    const ServerRuntime::DedicatedServerShutdownExecutionResult
        shutdownExecution =
            ServerRuntime::ExecuteDedicatedServerShutdown();
    const bool platformGameplayHaltedAfter =
        ServerRuntime::IsDedicatedServerGameplayHalted();
    ServerRuntime::StopDedicatedServerPlatformRuntime();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
        platformHostedRuntimeResult =
            ServerRuntime::StartDedicatedServerPlatformRuntime(platformState);
    ServerRuntime::NativeDedicatedServerSaveStub previousSaveStub = {};
    previousSaveStub.startupMode = "loaded";
    previousSaveStub.remoteCommands = 9;
    previousSaveStub.autosaveCompletions = 7;
    previousSaveStub.workerPendingWorldActionTicks = 1;
    previousSaveStub.workerPendingAutosaveCommands = 4;
    previousSaveStub.workerPendingSaveCommands = 2;
    previousSaveStub.workerPendingStopCommands = 3;
    previousSaveStub.workerPendingHaltCommands = 1;
    previousSaveStub.workerTickCount = 22;
    previousSaveStub.completedWorkerActions = 6;
    previousSaveStub.processedAutosaveCommands = 7;
    previousSaveStub.processedSaveCommands = 13;
    previousSaveStub.processedStopCommands = 5;
    previousSaveStub.processedHaltCommands = 6;
    previousSaveStub.lastQueuedCommandId = 17;
    previousSaveStub.activeCommandId = 15;
    previousSaveStub.activeCommandTicksRemaining = 2;
    previousSaveStub.activeCommandKind =
        ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Autosave;
    previousSaveStub.lastProcessedCommandId = 16;
    previousSaveStub.lastProcessedCommandKind =
        ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Stop;
    previousSaveStub.platformTickCount = 42;
    previousSaveStub.uptimeMs = 1337;
    previousSaveStub.hostSettings = 0xCAFEU;
    previousSaveStub.dedicatedNoLocalHostPlayer = false;
    previousSaveStub.worldSizeChunks = 160U;
    previousSaveStub.worldHellScale = 3U;
    previousSaveStub.onlineGame = true;
    previousSaveStub.privateGame = false;
    previousSaveStub.fakeLocalPlayerJoined = true;
    previousSaveStub.publicSlots = 16;
    previousSaveStub.privateSlots = 1;
    previousSaveStub.payloadChecksum = 0x123456789abcdef0ULL;
    previousSaveStub.saveGeneration = 11U;
    previousSaveStub.stateChecksum = 0x0fedcba987654321ULL;
    previousSaveStub.gameplayLoopIterations = 14U;
    ServerRuntime::RecordNativeDedicatedServerLoadedSaveMetadata(
        "NativeDesktop/GameHDD/SmokeSession.save",
        previousSaveStub);
    ServerRuntime::NativeDedicatedServerHostedGameRuntimeStubInitData
        nativeHostedStubInitData = {};
    ServerRuntime::PopulateDedicatedServerNetworkGameInitData(
        &nativeHostedStubInitData,
        hostedGamePlan.networkInitPlan);
    nativeHostedStubInitData.saveData = nativeHostedSaveData;
    ServerRuntime::ResetNativeDedicatedServerHostedGameSessionState();
    ServerRuntime::ResetDedicatedServerAutosaveTracker();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    const std::uint64_t nativeHostedStubTickBefore =
        ServerRuntime::GetDedicatedServerPlatformTickCount();
    const int nativeHostedStubResult =
        ServerRuntime::StartDedicatedServerHostedGameRuntime(
            hostedGamePlan,
            ServerRuntime::GetDedicatedServerHostedGameRuntimeThreadProc(),
            &nativeHostedStubInitData);
    const std::uint64_t nativeHostedStubTickAfter =
        ServerRuntime::GetDedicatedServerPlatformTickCount();
    const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        nativeHostedStubSnapshot =
            ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
    LceSleepMilliseconds(25);
    const std::uint64_t nativeHostedStubThreadTicks =
        ServerRuntime::GetNativeDedicatedServerHostedGameSessionThreadTicks();
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        nativeHostedSessionCoreSnapshot =
            ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
    ServerRuntime::EnableDedicatedServerSaveOnExit();
    ServerRuntime::HaltDedicatedServerGameplay();
    const bool nativeHostedStubHaltQueued =
        !ServerRuntime::IsDedicatedServerWorldActionIdle(0);
    DWORD nativeHostedStubExitCode = 0;
    const bool nativeHostedStubStopped =
        ServerRuntime::WaitForNativeDedicatedServerHostedGameSessionStop(
            INFINITE,
            &nativeHostedStubExitCode);
    const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        nativeHostedStubStoppedSnapshot =
            ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        nativeHostedSessionCoreStoppedSnapshot =
            ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
    ServerRuntime::StopDedicatedServerPlatformRuntime();
    const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
        restartedHostedRuntimeResult =
            ServerRuntime::StartDedicatedServerPlatformRuntime(platformState);
    ServerRuntime::ResetNativeDedicatedServerHostedGameSessionState();
    ServerRuntime::ResetDedicatedServerAutosaveTracker();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    ServerRuntime::NativeDedicatedServerHostedGameRuntimeStubInitData
        nativeHostedCoreStartupInitData = {};
    ServerRuntime::PopulateDedicatedServerNetworkGameInitData(
        &nativeHostedCoreStartupInitData,
        hostedGamePlan.networkInitPlan);
    ServerRuntime::PopulateDedicatedServerHostedGameRuntimeStubInitData(
        &nativeHostedCoreStartupInitData,
        hostedGamePlan);
    nativeHostedCoreStartupInitData.saveData = nullptr;
    const ServerRuntime::NativeDedicatedServerHostedGameCoreStartupResult
        nativeHostedCoreStartupResult =
            ServerRuntime::StartNativeDedicatedServerHostedGameCoreWithResult(
                &nativeHostedCoreStartupInitData);
    ServerRuntime::StopNativeDedicatedServerHostedGameSession();
    ServerRuntime::ResetNativeDedicatedServerHostedGameSessionState();
    ServerRuntime::ResetDedicatedServerAutosaveTracker();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    ServerRuntime::NativeDedicatedServerHostedGameRuntimeStubInitData
        nativeHostedCoreFrameInitData = {};
    ServerRuntime::PopulateDedicatedServerNetworkGameInitData(
        &nativeHostedCoreFrameInitData,
        hostedGamePlan.networkInitPlan);
    ServerRuntime::PopulateDedicatedServerHostedGameRuntimeStubInitData(
        &nativeHostedCoreFrameInitData,
        hostedGamePlan);
    nativeHostedCoreFrameInitData.saveData = nullptr;
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        nativeHostedCoreFrameStartupSnapshot =
            ServerRuntime::StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
                &nativeHostedCoreFrameInitData,
                0,
                0,
                LceGetMonotonicMilliseconds());
    const bool nativeHostedCoreFrameStarted =
        nativeHostedCoreFrameStartupSnapshot.payloadValidated;
    ServerRuntime::RequestNativeDedicatedServerHostedGameSessionAutosave(
        2,
        LceGetMonotonicMilliseconds());
    const ServerRuntime::NativeDedicatedServerHostedGameCoreFrameResult
        nativeHostedCoreFrameFirst =
            ServerRuntime::TickNativeDedicatedServerHostedGameCoreFrameWithResult();
    const ServerRuntime::NativeDedicatedServerHostedGameCoreFrameResult
        nativeHostedCoreFrameSecond =
            ServerRuntime::TickNativeDedicatedServerHostedGameCoreFrameWithResult();
    ServerRuntime::StopNativeDedicatedServerHostedGameSession();
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        nativeHostedCoreFrameStoppedSnapshot =
            ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
    ServerRuntime::ResetNativeDedicatedServerHostedGameSessionState();
    ServerRuntime::ResetDedicatedServerAutosaveTracker();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    ServerRuntime::ResetNativeDedicatedServerHostedGameSessionState();
    ServerRuntime::ResetDedicatedServerAutosaveTracker();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    ServerRuntime::SetDedicatedServerAppShutdownRequested(false);
    g_nativeHostedCoreHookSmokeContext = {};
    g_nativeHostedCoreHookSmokeContext.requestShutdownOnReady = true;
    ServerRuntime::NativeDedicatedServerHostedGameRuntimeStubInitData
        nativeHostedCoreInitData = {};
    ServerRuntime::PopulateDedicatedServerNetworkGameInitData(
        &nativeHostedCoreInitData,
        hostedGamePlan.networkInitPlan);
    nativeHostedCoreInitData.saveData = nullptr;
    ServerRuntime::NativeDedicatedServerHostedGameCoreHooks
        nativeHostedCoreHooks = {};
    nativeHostedCoreHooks.onThreadReady = &NativeHostedCoreReadyHook;
    nativeHostedCoreHooks.onThreadStopped = &NativeHostedCoreStoppedHook;
    const ServerRuntime::NativeDedicatedServerHostedGameCoreRunResult
        nativeHostedCoreRunResult =
            ServerRuntime::RunNativeDedicatedServerHostedGameCoreWithResult(
                &nativeHostedCoreInitData,
                nativeHostedCoreHooks);
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        nativeHostedCoreSnapshot = nativeHostedCoreRunResult
            .finalState.sessionSnapshot;
    const int hostedGameRuntimeNullThreadResult =
        ServerRuntime::StartDedicatedServerHostedGameRuntime(
            hostedGamePlan,
            nullptr,
            nullptr);
    const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        hostedGameRuntimeNullThreadSnapshot =
            ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
    const std::uint64_t hostedGameRuntimeTickBefore =
        ServerRuntime::GetDedicatedServerPlatformTickCount();
    int hostedGameRuntimeSleepThreadValue = 0;
    const int hostedGameRuntimeSleepResult =
        ServerRuntime::StartDedicatedServerHostedGameRuntime(
            hostedGamePlan,
            &HostedGameRuntimeSleepSmokeThreadProc,
            &hostedGameRuntimeSleepThreadValue);
    const std::uint64_t hostedGameRuntimeTickAfter =
        ServerRuntime::GetDedicatedServerPlatformTickCount();
    int hostedGameRuntimeThreadValue = 0;
    const int hostedGameRuntimeResult =
        ServerRuntime::StartDedicatedServerHostedGameRuntime(
            hostedGamePlan,
            &HostedGameRuntimeSmokeThreadProc,
            &hostedGameRuntimeThreadValue);
    SmokeNetworkGameInitData hostedGameStartupInitData = {};
    const ServerRuntime::DedicatedServerHostedGameStartupExecutionResult
        hostedGameStartupExecution =
            ServerRuntime::ExecuteDedicatedServerHostedGameStartup(
                hostedGamePlan,
                &HostedGameStartupSmokeThreadProc,
                &hostedGameStartupInitData);
    const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        hostedGameRuntimeSnapshot =
            ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
    ServerRuntime::ResetNativeDedicatedServerHostedGameSessionState();
    ServerRuntime::DedicatedServerHostedGameRuntimeSessionContext
        hostedGameSessionContext = {};
    hostedGameSessionContext.worldName = "Smoke Session";
    hostedGameSessionContext.worldSaveId = "SMOKE_SESSION";
    hostedGameSessionContext.savePath =
        "NativeDesktop/GameHDD/SMOKE_SESSION.save";
    hostedGameSessionContext.storageRoot = "NativeDesktop/GameHDD";
    hostedGameSessionContext.hostName = "SmokeHost";
    hostedGameSessionContext.bindIp = "127.0.0.1";
    hostedGameSessionContext.configuredPort = 25565;
    hostedGameSessionContext.listenerPort = 19132;
    ServerRuntime::RecordDedicatedServerHostedGameRuntimeSessionContext(
        hostedGameSessionContext,
        1000);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionContext(
        hostedGameSessionContext.worldName,
        hostedGameSessionContext.worldSaveId,
        hostedGameSessionContext.savePath,
        hostedGameSessionContext.storageRoot,
        hostedGameSessionContext.hostName,
        hostedGameSessionContext.bindIp,
        hostedGameSessionContext.configuredPort,
        hostedGameSessionContext.listenerPort,
        1000);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionActivation(
        0,
        true,
        false,
        sessionConfig.networkMaxPlayers,
        0,
        true);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
        0,
        true,
        LceGetMonotonicMilliseconds());
    ServerRuntime::DedicatedServerHostedGameRuntimeSessionSummary
        hostedGameSessionSummary = {};
    hostedGameSessionSummary.initialSaveRequested = true;
    hostedGameSessionSummary.initialSaveCompleted = true;
    hostedGameSessionSummary.sessionCompleted = true;
    hostedGameSessionSummary.requestedAppShutdown = true;
    hostedGameSessionSummary.shutdownHaltedGameplay = true;
    hostedGameSessionSummary.gameplayLoopIterations = 8;
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionActivity(
        2,
        3,
        false);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionPlatformState(
        4,
        6);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionWorkerState(
        9,
        8,
        10,
        11,
        12,
        22,
        32,
        12,
        42,
        52,
        62,
        99,
        97,
        3,
        ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Autosave,
        98,
        ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Stop);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
        8,
        true,
        true,
        true);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionSummary(
        true,
        true,
        false,
        true,
        true,
        true);
    ServerRuntime::ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        "NativeDesktop/GameHDD/SMOKE_SESSION.save",
        77,
        5);
    ServerRuntime::ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
        1100);
    const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        hostedGameSessionSnapshot =
            ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        nativeHostedSessionObservedSnapshot =
            ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
    ServerRuntime::StopNativeDedicatedServerHostedGameSession(1200);
    ServerRuntime::ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
        1200);
    const ServerRuntime::NativeDedicatedServerHostedGameSessionSnapshot
        nativeHostedSessionStoppedSnapshot =
            ServerRuntime::GetNativeDedicatedServerHostedGameSessionSnapshot();
    const ServerRuntime::DedicatedServerHostedGameRuntimeSnapshot
        hostedGameStoppedSnapshot =
            ServerRuntime::GetDedicatedServerHostedGameRuntimeSnapshot();
    const bool hostedGameSessionCurrentSnapshotOk =
        hostedGameSessionSnapshot.sessionActive &&
        hostedGameSessionSnapshot.phase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_ShutdownRequested &&
        hostedGameSessionSnapshot.worldName == "Smoke Session" &&
        hostedGameSessionSnapshot.worldSaveId == "SMOKE_SESSION" &&
        hostedGameSessionSnapshot.savePath ==
            "NativeDesktop/GameHDD/SMOKE_SESSION.save" &&
        hostedGameSessionSnapshot.storageRoot == "NativeDesktop/GameHDD" &&
        hostedGameSessionSnapshot.hostName == "SmokeHost" &&
        hostedGameSessionSnapshot.bindIp == "127.0.0.1" &&
        hostedGameSessionSnapshot.configuredPort == 25565 &&
        hostedGameSessionSnapshot.listenerPort == 19132 &&
        hostedGameSessionSnapshot.acceptedConnections == 2 &&
        hostedGameSessionSnapshot.remoteCommands == 3 &&
        hostedGameSessionSnapshot.autosaveRequests == 4 &&
        hostedGameSessionSnapshot.autosaveCompletions == 5 &&
        hostedGameSessionSnapshot.workerPendingWorldActionTicks == 9 &&
        hostedGameSessionSnapshot.workerPendingAutosaveCommands == 8 &&
        hostedGameSessionSnapshot.workerPendingSaveCommands == 10 &&
        hostedGameSessionSnapshot.workerPendingStopCommands == 11 &&
        hostedGameSessionSnapshot.workerPendingHaltCommands == 12 &&
        hostedGameSessionSnapshot.workerTickCount == 22 &&
        hostedGameSessionSnapshot.completedWorkerActions == 32 &&
        hostedGameSessionSnapshot.processedAutosaveCommands == 12 &&
        hostedGameSessionSnapshot.processedSaveCommands == 42 &&
        hostedGameSessionSnapshot.processedStopCommands == 52 &&
        hostedGameSessionSnapshot.processedHaltCommands == 62 &&
        hostedGameSessionSnapshot.lastQueuedCommandId == 99 &&
        hostedGameSessionSnapshot.activeCommandId == 97 &&
        hostedGameSessionSnapshot.activeCommandTicksRemaining == 3 &&
        hostedGameSessionSnapshot.activeCommandKind ==
            ServerRuntime::
                eNativeDedicatedServerHostedGameWorkerCommand_Autosave &&
        hostedGameSessionSnapshot.lastProcessedCommandId == 98 &&
        hostedGameSessionSnapshot.lastProcessedCommandKind ==
            ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Stop &&
        hostedGameSessionSnapshot.platformTickCount == 6 &&
        !hostedGameSessionSnapshot.worldActionIdle &&
        hostedGameSessionSnapshot.appShutdownRequested &&
        hostedGameSessionSnapshot.gameplayHalted &&
        hostedGameSessionSnapshot.stopSignalValid &&
        hostedGameSessionSnapshot.initialSaveRequested &&
        hostedGameSessionSnapshot.initialSaveCompleted &&
        !hostedGameSessionSnapshot.initialSaveTimedOut &&
        hostedGameSessionSnapshot.sessionCompleted &&
        hostedGameSessionSnapshot.requestedAppShutdown &&
        hostedGameSessionSnapshot.shutdownHaltedGameplay &&
        hostedGameSessionSnapshot.gameplayLoopIterations == 8 &&
        hostedGameSessionSnapshot.lastPersistedSavePath ==
            "NativeDesktop/GameHDD/SMOKE_SESSION.save" &&
        hostedGameSessionSnapshot.lastPersistedFileTime == 77 &&
        hostedGameSessionSnapshot.lastPersistedAutosaveCompletions == 5 &&
        hostedGameSessionSnapshot.uptimeMs == 100;
    const bool hostedGameSessionObservedIdentityOk =
        nativeHostedSessionObservedSnapshot.worldName == "Smoke Session" &&
        nativeHostedSessionObservedSnapshot.worldSaveId == "SMOKE_SESSION" &&
        nativeHostedSessionObservedSnapshot.savePath ==
            "NativeDesktop/GameHDD/SMOKE_SESSION.save" &&
        nativeHostedSessionObservedSnapshot.storageRoot ==
            "NativeDesktop/GameHDD" &&
        nativeHostedSessionObservedSnapshot.hostName == "SmokeHost" &&
        nativeHostedSessionObservedSnapshot.bindIp == "127.0.0.1" &&
        nativeHostedSessionObservedSnapshot.threadInvoked &&
        nativeHostedSessionObservedSnapshot.startupResult == 0 &&
        nativeHostedSessionObservedSnapshot.localUsersMask == 0 &&
        nativeHostedSessionObservedSnapshot.onlineGame &&
        !nativeHostedSessionObservedSnapshot.privateGame &&
        nativeHostedSessionObservedSnapshot.publicSlots ==
            sessionConfig.networkMaxPlayers &&
        nativeHostedSessionObservedSnapshot.privateSlots == 0 &&
        nativeHostedSessionObservedSnapshot.fakeLocalPlayerJoined &&
        nativeHostedSessionObservedSnapshot.configuredPort == 25565 &&
        nativeHostedSessionObservedSnapshot.listenerPort == 19132 &&
        nativeHostedSessionObservedSnapshot.sessionStartMs == 1000;
    const bool hostedGameSessionObservedWorkerOk =
        nativeHostedSessionObservedSnapshot.acceptedConnections == 2 &&
        nativeHostedSessionObservedSnapshot.remoteCommands == 3 &&
        nativeHostedSessionObservedSnapshot.autosaveRequests == 4 &&
        nativeHostedSessionObservedSnapshot.workerPendingWorldActionTicks == 9 &&
        nativeHostedSessionObservedSnapshot.workerPendingAutosaveCommands == 8 &&
        nativeHostedSessionObservedSnapshot.workerPendingSaveCommands == 10 &&
        nativeHostedSessionObservedSnapshot.workerPendingStopCommands == 11 &&
        nativeHostedSessionObservedSnapshot.workerPendingHaltCommands == 12 &&
        nativeHostedSessionObservedSnapshot.workerTickCount == 22 &&
        nativeHostedSessionObservedSnapshot.completedWorkerActions == 32 &&
        nativeHostedSessionObservedSnapshot.processedAutosaveCommands == 12 &&
        nativeHostedSessionObservedSnapshot.processedSaveCommands == 42 &&
        nativeHostedSessionObservedSnapshot.processedStopCommands == 52 &&
        nativeHostedSessionObservedSnapshot.processedHaltCommands == 62 &&
        nativeHostedSessionObservedSnapshot.lastQueuedCommandId == 99 &&
        nativeHostedSessionObservedSnapshot.activeCommandId == 97 &&
        nativeHostedSessionObservedSnapshot.activeCommandTicksRemaining == 3 &&
        nativeHostedSessionObservedSnapshot.activeCommandKind ==
            ServerRuntime::
                eNativeDedicatedServerHostedGameWorkerCommand_Autosave &&
        nativeHostedSessionObservedSnapshot.lastProcessedCommandId == 98 &&
        nativeHostedSessionObservedSnapshot.lastProcessedCommandKind ==
            ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Stop &&
        nativeHostedSessionObservedSnapshot.gameplayLoopIterations == 8 &&
        nativeHostedSessionObservedSnapshot.platformTickCount == 6 &&
        nativeHostedSessionObservedSnapshot.lastPersistedSavePath ==
            "NativeDesktop/GameHDD/SMOKE_SESSION.save" &&
        nativeHostedSessionObservedSnapshot.lastPersistedFileTime == 77 &&
        nativeHostedSessionObservedSnapshot
            .lastPersistedAutosaveCompletions == 5 &&
        !nativeHostedSessionObservedSnapshot.worldActionIdle;
    const bool hostedGameSessionObservedLifecycleOk =
        nativeHostedSessionObservedSnapshot.active &&
        !nativeHostedSessionObservedSnapshot.worldActionIdle &&
        nativeHostedSessionObservedSnapshot.appShutdownRequested &&
        nativeHostedSessionObservedSnapshot.stopSignalValid &&
        nativeHostedSessionObservedSnapshot.sessionCompleted &&
        nativeHostedSessionObservedSnapshot.requestedAppShutdown &&
        nativeHostedSessionObservedSnapshot.runtimePhase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_ShutdownRequested &&
        nativeHostedSessionObservedSnapshot.stoppedMs == 0 &&
        nativeHostedSessionObservedSnapshot.stateChecksum != 0U;
    const bool hostedGameSessionObservedProjectionOk =
        hostedGameSessionObservedIdentityOk &&
        hostedGameSessionObservedWorkerOk &&
        hostedGameSessionObservedLifecycleOk;
    const bool hostedGameSessionStoppedOk =
        !hostedGameStoppedSnapshot.sessionActive &&
        hostedGameStoppedSnapshot.phase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Stopped &&
        hostedGameStoppedSnapshot.stoppedMs == 1200 &&
        hostedGameStoppedSnapshot.uptimeMs == 200 &&
        !nativeHostedSessionStoppedSnapshot.active &&
        nativeHostedSessionStoppedSnapshot.runtimePhase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Stopped &&
        nativeHostedSessionStoppedSnapshot.stoppedMs == 1200 &&
        nativeHostedSessionStoppedSnapshot.lastPersistedSavePath ==
            "NativeDesktop/GameHDD/SMOKE_SESSION.save";
    const bool hostedGameSessionOk =
        hostedGameSessionCurrentSnapshotOk &&
        hostedGameSessionObservedProjectionOk &&
        hostedGameSessionStoppedOk;
    ServerRuntime::StopDedicatedServerPlatformRuntime();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
        platformSessionRuntimeResult =
            ServerRuntime::StartDedicatedServerPlatformRuntime(platformState);
    GameplayLoopRunPollContext sessionPollContext = {};
    const ServerRuntime::DedicatedServerSessionExecutionResult
        sessionExecution =
            ServerRuntime::ExecuteDedicatedServerSession(
                createdWorldPlan,
                runtimeProperties,
                0,
                &GameplayLoopRunPoll,
                &sessionPollContext,
                0,
                0);
    const bool sessionGameplayHalted =
        ServerRuntime::IsDedicatedServerGameplayHalted();
    ServerRuntime::StopDedicatedServerPlatformRuntime();
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
        platformRunRuntimeResult =
            ServerRuntime::StartDedicatedServerPlatformRuntime(platformState);
    ServerRuntime::ServerPropertiesConfig runExecutionProperties =
        sessionProperties;
    runExecutionProperties.worldName = L"Run Execution";
    runExecutionProperties.worldSaveId = "WORLD";
    runExecutionProperties.autosaveIntervalSeconds = 45;
    SmokeNetworkGameInitData runExecutionInitData = {};
    int runAfterStartupCount = 0;
    int runBeforeSessionCount = 0;
    int runAfterSessionCount = 0;
    GameplayLoopRunPollContext runExecutionPollContext = {};
    ServerRuntime::DedicatedServerRunHooks runHooks = {};
    runHooks.afterStartupProc = &IncrementHookCount;
    runHooks.afterStartupContext = &runAfterStartupCount;
    runHooks.beforeSessionProc = &IncrementHookCount;
    runHooks.beforeSessionContext = &runBeforeSessionCount;
    runHooks.pollProc = &GameplayLoopRunPoll;
    runHooks.pollContext = &runExecutionPollContext;
    runHooks.afterSessionProc = &IncrementHookCount;
    runHooks.afterSessionContext = &runAfterSessionCount;
    const ServerRuntime::DedicatedServerRunExecutionResult
        runExecution =
            ServerRuntime::ExecuteDedicatedServerRun(
                sessionDedicatedConfig,
                &runExecutionProperties,
                createdWorldBootstrap,
                0,
                9999,
                &HostedGameStartupSmokeThreadProc,
                &runExecutionInitData,
                runHooks,
                0,
                0);
    const bool runExecutionGameplayHalted =
        ServerRuntime::IsDedicatedServerGameplayHalted();
    ServerRuntime::StopDedicatedServerPlatformRuntime();
    const std::uint64_t defaultAutosaveMs =
        ServerRuntime::GetDedicatedServerAutosaveIntervalMs(
            dedicatedProperties);
    const std::uint64_t configuredAutosaveMs =
        ServerRuntime::GetDedicatedServerAutosaveIntervalMs(
            runtimeProperties);
    const std::uint64_t nextAutosaveTick =
        ServerRuntime::ComputeNextDedicatedServerAutosaveDeadlineMs(
            1000,
            runtimeProperties);
    ServerRuntime::DedicatedServerAutosaveState autosaveState =
        ServerRuntime::CreateDedicatedServerAutosaveState(
            1000,
            runtimeProperties);
    const bool autosaveTriggerBefore =
        !ServerRuntime::ShouldTriggerDedicatedServerAutosave(
            autosaveState,
            45000);
    const bool autosaveTriggerAtDeadline =
        ServerRuntime::ShouldTriggerDedicatedServerAutosave(
            autosaveState,
            46000);
    ServerRuntime::MarkDedicatedServerAutosaveRequested(
        &autosaveState,
        46000,
        runtimeProperties);
    const bool autosaveRequested = autosaveState.autosaveRequested;
    const bool autosaveNextAdvanced = autosaveState.nextTickMs == 91000U;
    const ServerRuntime::DedicatedServerAutosaveLoopPlan autosaveCompletePlan =
        ServerRuntime::BuildDedicatedServerAutosaveLoopPlan(
            autosaveState,
            true,
            47000);
    ServerRuntime::MarkDedicatedServerAutosaveCompleted(&autosaveState);
    const bool autosaveCompleted = !autosaveState.autosaveRequested;
    const ServerRuntime::DedicatedServerAutosaveLoopPlan autosaveRequestPlan =
        ServerRuntime::BuildDedicatedServerAutosaveLoopPlan(
            autosaveState,
            true,
            91000);
    const ServerRuntime::DedicatedServerAutosaveLoopPlan autosaveDelayPlan =
        ServerRuntime::BuildDedicatedServerAutosaveLoopPlan(
            autosaveState,
            false,
            91000);
    ServerRuntime::DedicatedServerAutosaveState autosaveCatchupState =
        autosaveState;
    autosaveCatchupState.autosaveRequested = true;
    const ServerRuntime::DedicatedServerAutosaveLoopPlan autosaveCatchupPlan =
        ServerRuntime::BuildDedicatedServerAutosaveLoopPlan(
            autosaveCatchupState,
            true,
            91000);
    const char* storagePlatformDirectory =
        ServerRuntime::GetServerStoragePlatformDirectory();
    const std::string storageGameHddRoot =
        ServerRuntime::GetServerGameHddRootPath();
    std::string overriddenStorageRoot;
    {
        ScopedEnvVar storageRootOverride(
            "MINECRAFT_SERVER_STORAGE_ROOT",
            "/tmp/minecraft-native-storage-smoke");
        overriddenStorageRoot = ServerRuntime::GetServerGameHddRootPath();
    }
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
    char bootstrapArg0[] = "Minecraft.Server.NativeBootstrap";
    char bootstrapArg1[] = "-port";
    char bootstrapArg2[] = "25578";
    char bootstrapArg3[] = "-bind";
    char bootstrapArg4[] = "127.0.0.1";
    char bootstrapArg5[] = "-name";
    char bootstrapArg6[] = "BootstrapSmoke";
    char bootstrapArg7[] = "-loglevel";
    char bootstrapArg8[] = "warn";
    char* bootstrapArgv[] = {
        bootstrapArg0,
        bootstrapArg1,
        bootstrapArg2,
        bootstrapArg3,
        bootstrapArg4,
        bootstrapArg5,
        bootstrapArg6,
        bootstrapArg7,
        bootstrapArg8
    };
    ServerRuntime::DedicatedServerBootstrapContext bootstrapSmokeContext = {};
    std::string bootstrapSmokeError;
    const ServerRuntime::EDedicatedServerBootstrapStatus bootstrapStatus =
        ServerRuntime::PrepareDedicatedServerBootstrapContext(
            static_cast<int>(sizeof(bootstrapArgv) / sizeof(bootstrapArgv[0])),
            bootstrapArgv,
            &bootstrapSmokeContext,
            &bootstrapSmokeError);
    const bool bootstrapPrepared =
        bootstrapStatus == ServerRuntime::eDedicatedServerBootstrap_Ready &&
        bootstrapSmokeError.empty();
    bool bootstrapEnvironmentReady = false;
    bool bootstrapAccessReady = false;
    bool bootstrapAccessShutdown = false;
    if (bootstrapPrepared)
    {
        bootstrapEnvironmentReady =
            ServerRuntime::InitializeDedicatedServerBootstrapEnvironment(
                &bootstrapSmokeContext,
                &bootstrapSmokeError);
        bootstrapAccessReady =
            bootstrapEnvironmentReady &&
            ServerRuntime::Access::IsInitialized();
        ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
            &bootstrapSmokeContext);
        bootstrapAccessShutdown =
            !ServerRuntime::Access::IsInitialized() &&
            !bootstrapSmokeContext.accessInitialized;
    }
    std::filesystem::current_path(smokeWorkingDirectory);
    const bool bootstrapRestoredDirectory =
        std::filesystem::current_path() == smokeWorkingDirectory;
    ServerRuntime::DedicatedServerBootstrapContext socketBootstrapContext = {};
    socketBootstrapContext.runtimeState.bindIp = "127.0.0.1";
    socketBootstrapContext.runtimeState.multiplayerPort = 0;
    ServerRuntime::DedicatedServerSocketBootstrapState socketBootstrapState =
        {};
    std::string socketBootstrapError;
    const bool socketBootstrapStarted =
        ServerRuntime::StartDedicatedServerSocketBootstrap(
            socketBootstrapContext,
            &socketBootstrapState,
            &socketBootstrapError);
    LceSocketHandle socketBootstrapClient = LCE_INVALID_SOCKET;
    LceSocketHandle socketBootstrapAccepted = LCE_INVALID_SOCKET;
    char socketBootstrapAcceptedIp[64] = {};
    int socketBootstrapAcceptedPort = -1;
    bool socketBootstrapConnected = false;
    bool socketBootstrapAcceptedOk = false;
    if (socketBootstrapStarted)
    {
        socketBootstrapClient = LceNetOpenTcpSocket();
        socketBootstrapConnected =
            socketBootstrapClient != LCE_INVALID_SOCKET &&
            LceNetConnectIpv4(
                socketBootstrapClient,
                "127.0.0.1",
                socketBootstrapState.boundPort);
        if (socketBootstrapConnected)
        {
            socketBootstrapAccepted = LceNetAcceptIpv4(
                socketBootstrapState.listener,
                socketBootstrapAcceptedIp,
                sizeof(socketBootstrapAcceptedIp),
                &socketBootstrapAcceptedPort);
            socketBootstrapAcceptedOk =
                socketBootstrapAccepted != LCE_INVALID_SOCKET &&
                socketBootstrapAcceptedIp[0] != '\0' &&
                socketBootstrapAcceptedPort > 0;
        }
    }
    if (socketBootstrapAccepted != LCE_INVALID_SOCKET)
    {
        LceNetCloseSocket(socketBootstrapAccepted);
    }
    if (socketBootstrapClient != LCE_INVALID_SOCKET)
    {
        LceNetCloseSocket(socketBootstrapClient);
    }
    const int socketBootstrapBoundPort = socketBootstrapState.boundPort;
    ServerRuntime::StopDedicatedServerSocketBootstrap(&socketBootstrapState);
    const bool socketBootstrapStopped =
        socketBootstrapState.listener == LCE_INVALID_SOCKET &&
        socketBootstrapState.boundPort == -1;
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
    timer.PrintElapsedTime(L"smoke");

    const std::uint64_t monotonicMsAfter = LceGetMonotonicMilliseconds();
    const std::uint64_t monotonicNsAfter = LceGetMonotonicNanoseconds();
    const std::uint64_t deltaMs = monotonicMsAfter - monotonicMsBefore;
    const std::uint64_t deltaNs = monotonicNsAfter - monotonicNsBefore;

    printf("path=%s\n", path);
    printf("exists=%d file=%d directory=%d\n", exists, isFile, isDirectory);
    printf("smoke_directory_ready=%d created=%d\n",
        smokeDirectoryReady,
        createdDirectory);
    printf("executable_directory_ok=%d executable_directory=%s\n",
        resolvedExecutableDirectory,
        executableDirectory.c_str());
    printf("net_initialized=%d ipv4_literal=%d ipv6_literal=%d invalid_literal=%d\n",
        netInitialized,
        ipv4Literal,
        ipv6Literal,
        invalidLiteral);
    printf("udp_receiver=%d,%d,%d port=%d udp_sender_broadcast=%d "
        "udp_sent=%d udp_recv=%d udp_payload_ok=%d udp_sender_ip=%s "
        "udp_sender_port=%d\n",
        udpReceiver != LCE_INVALID_SOCKET,
        udpReceiverReuse,
        udpReceiverTimeout && udpReceiverBound,
        udpReceiverPort,
        udpSenderBroadcast,
        udpSent,
        udpRecv,
        udpPayloadOk,
        udpSenderIp,
        udpSenderPort);
    printf("tcp_listener=%d,%d,%d port=%d resolved=%d ip=%s listening=%d tcp_client=%d,%d,%d "
        "connected=%d tcp_accepted=%d,%d,%d ip=%s port=%d "
        "server_recv=%d server_payload_ok=%d client_recv=%d "
        "client_payload_ok=%d\n",
        tcpListener != LCE_INVALID_SOCKET,
        tcpListenerReuse,
        tcpListenerBound,
        tcpListenerPort,
        tcpResolvedLocalhost,
        resolvedLocalhost,
        tcpListening,
        tcpClient != LCE_INVALID_SOCKET,
        tcpClientNoDelay,
        tcpClientTimeout,
        tcpConnected,
        tcpAccepted != LCE_INVALID_SOCKET,
        tcpAcceptedNoDelay,
        tcpAcceptedTimeout,
        tcpAcceptedIp,
        tcpAcceptedPort,
        tcpServerReceived,
        tcpServerPayloadOk,
        tcpClientReceived,
        tcpClientPayloadOk);
    printf("lan_broadcast=%d lan_decode=%d lan_added=%d lan_updated_added=%d "
        "lan_count=%zu lan_expired=%zu lan_max_players=%u lan_wire_size=%d\n",
        lanBroadcastBuilt,
        lanBroadcastDecoded,
        lanAdded,
        lanUpdatedAdded,
        lanSessions.size(),
        expiredLanSessions.size(),
        static_cast<unsigned int>(lanBroadcast.maxPlayers),
        lanWireSizeOk);
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
    printf("world_save_selection=%d save_id=%d by_save_id=%d "
        "name_fallback=%d no_match=%d\n",
        worldSaveMatchById.matchedIndex == 1 &&
            worldSaveMatchById.matchedBySaveId &&
            worldSaveMatchByName.matchedIndex == 1 &&
            !worldSaveMatchByName.matchedBySaveId &&
            worldSaveNoMatch.matchedIndex == -1 &&
            !worldSaveNoMatch.matchedBySaveId,
        worldSaveMatchById.matchedIndex,
        worldSaveMatchById.matchedBySaveId,
        worldSaveMatchByName.matchedIndex,
        worldSaveNoMatch.matchedIndex);
    printf("world_bootstrap_target=%d default=%ls configured=%ls "
        "loaded=%d created=%d failed=%d\n",
        defaultWorldTarget.worldName == L"world" &&
            defaultWorldTarget.saveId.empty() &&
            configuredWorldTarget.worldName == L"Smoke World" &&
            configuredWorldTarget.saveId == "SMOKE_WORLD" &&
            loadedWorldTargetBootstrap.status ==
                ServerRuntime::eWorldBootstrap_Loaded &&
            loadedWorldTargetBootstrap.saveData == fakeBootstrapSaveData &&
            loadedWorldTargetBootstrap.resolvedSaveId == "resolved_world" &&
            createdWorldTargetBootstrap.status ==
                ServerRuntime::eWorldBootstrap_CreatedNew &&
            createdWorldTargetBootstrap.saveData == NULL &&
            createdWorldTargetBootstrap.resolvedSaveId == "SMOKE_WORLD" &&
            failedWorldTargetBootstrap.status ==
                ServerRuntime::eWorldBootstrap_Failed &&
            failedWorldTargetBootstrap.saveData == NULL &&
            failedWorldTargetBootstrap.resolvedSaveId.empty(),
        defaultWorldTarget.worldName.c_str(),
        configuredWorldTarget.worldName.c_str(),
        loadedWorldTargetBootstrap.status,
        createdWorldTargetBootstrap.status,
        failedWorldTargetBootstrap.status);
    printf("world_storage_plan=%d configured=%s loaded=%s fallback=%s "
        "created_reset=%d applied=%d/%d/%d final=%ls/%s resets=%d\n",
        configuredWorldStoragePlan.worldName == L"Smoke World" &&
            configuredWorldStoragePlan.saveId == "SMOKE_WORLD" &&
            !configuredWorldStoragePlan.shouldResetSaveData &&
            loadedWorldStoragePlan.saveId == "resolved_world" &&
            !loadedWorldStoragePlan.shouldResetSaveData &&
            fallbackLoadedWorldStoragePlan.saveId == "SMOKE_WORLD" &&
            !fallbackLoadedWorldStoragePlan.shouldResetSaveData &&
            createdWorldStoragePlan.saveId == "SMOKE_WORLD" &&
            createdWorldStoragePlan.shouldResetSaveData &&
            appliedConfiguredWorldStorage &&
            appliedLoadedWorldStorage &&
            appliedCreatedWorldStorage &&
            worldStorageHookContext.appliedWorldName == L"Smoke World" &&
            worldStorageHookContext.appliedSaveId == "SMOKE_WORLD" &&
            worldStorageHookContext.resetCount == 1,
        configuredWorldStoragePlan.saveId.c_str(),
        loadedWorldStoragePlan.saveId.c_str(),
        fallbackLoadedWorldStoragePlan.saveId.c_str(),
        createdWorldStoragePlan.shouldResetSaveData,
        appliedConfiguredWorldStorage,
        appliedLoadedWorldStorage,
        appliedCreatedWorldStorage,
        worldStorageHookContext.appliedWorldName.c_str(),
        worldStorageHookContext.appliedSaveId.c_str(),
        worldStorageHookContext.resetCount);
    printf("world_load_pipeline=%d loaded=%d matched=%d bytes=%zu "
        "owned=%d not_found=%d corrupt=%d\n",
        worldLoadPipelineStatus ==
                ServerRuntime::eDedicatedServerWorldLoad_Loaded &&
            worldLoadPipelineContext.beginQueryCount == 1 &&
            worldLoadPipelineContext.beginLoadCount == 1 &&
            worldLoadPipelineContext.matchedIndex == 1 &&
            worldLoadPayload.matchedTitle == L"Smoke World" &&
            worldLoadPayload.matchedFilename == L"resolved_world" &&
            worldLoadPayload.resolvedSaveId == "resolved_world" &&
            worldLoadPayload.bytes.size() == 4 &&
            ownedLoadSaveDataCopied &&
            worldLoadPipelineNotFoundStatus ==
                ServerRuntime::eDedicatedServerWorldLoad_NotFound &&
            worldLoadPipelineNotFoundContext.beginQueryCount == 1 &&
            worldLoadPipelineNotFoundContext.beginLoadCount == 0 &&
            worldLoadPipelineCorruptStatus ==
                ServerRuntime::eDedicatedServerWorldLoad_Failed &&
            worldLoadPipelineCorruptContext.beginQueryCount == 1 &&
            worldLoadPipelineCorruptContext.beginLoadCount == 1,
        worldLoadPipelineStatus,
        worldLoadPipelineContext.matchedIndex,
        worldLoadPayload.bytes.size(),
        ownedLoadSaveDataCopied,
        worldLoadPipelineNotFoundStatus,
        worldLoadPipelineCorruptStatus);
    printf("dedicated_defaults=%d dedicated_props=%d dedicated_cli=%d "
        "dedicated_help=%d dedicated_ephemeral=%d "
        "dedicated_invalid=%d usage_lines=%zu storage_runtime=%d "
        "storage_runtime_destroyed=%d\n",
        defaultDedicatedConfig.port == 25565 &&
            std::strcmp(defaultDedicatedConfig.bindIP, "0.0.0.0") == 0 &&
            std::strcmp(defaultDedicatedConfig.name, "DedicatedServer") == 0 &&
            defaultDedicatedConfig.maxPlayers == 256 &&
            !defaultDedicatedConfig.hasSeed &&
            defaultDedicatedConfig.logLevel ==
                ServerRuntime::eServerLogLevel_Info,
        propertyDedicatedConfig.port == dedicatedProperties.serverPort &&
            std::strcmp(
                propertyDedicatedConfig.bindIP,
                dedicatedProperties.serverIp.c_str()) == 0 &&
            std::strcmp(
                propertyDedicatedConfig.name,
                dedicatedProperties.serverName.c_str()) == 0 &&
            propertyDedicatedConfig.maxPlayers ==
                dedicatedProperties.maxPlayers &&
            propertyDedicatedConfig.worldSize ==
                dedicatedProperties.worldSize &&
            propertyDedicatedConfig.worldSizeChunks ==
                dedicatedProperties.worldSizeChunks &&
            propertyDedicatedConfig.worldHellScale ==
                dedicatedProperties.worldHellScale &&
            propertyDedicatedConfig.hasSeed &&
            propertyDedicatedConfig.seed == dedicatedProperties.seed &&
            propertyDedicatedConfig.logLevel ==
                dedicatedProperties.logLevel,
        dedicatedParseOk &&
            dedicatedParseError.empty() &&
            cliDedicatedConfig.port == 25571 &&
            std::strcmp(cliDedicatedConfig.bindIP, "127.0.0.1") == 0 &&
            std::strcmp(cliDedicatedConfig.name, "NativeCli") == 0 &&
            cliDedicatedConfig.maxPlayers == 32 &&
            cliDedicatedConfig.hasSeed &&
            cliDedicatedConfig.seed == 123456789LL &&
            cliDedicatedConfig.logLevel ==
                ServerRuntime::eServerLogLevel_Debug,
        dedicatedHelpOk &&
            dedicatedHelpError.empty() &&
            helpDedicatedConfig.showHelp,
        dedicatedEphemeralOk &&
            dedicatedEphemeralError.empty() &&
            ephemeralDedicatedConfig.port == 0,
        !dedicatedInvalidOk &&
            !dedicatedInvalidError.empty(),
        dedicatedUsageLines.size(),
        nativeWorldLoadStorageRuntimeReady,
        nativeWorldLoadStorageRuntimeDestroyed);
    printf("session_config=%d host_settings=0x%08x network_max_players=%u "
        "seed=%lld size=%u hell=%u app_plan=%d\n",
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_DIFFICULTY) ==
                2U &&
            ((sessionConfig.hostSettings &
                    GAME_HOST_OPTION_BITMASK_GAMETYPE) >>
                4U) == 1U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_GAMERTAGS) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_LEVELTYPE) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_STRUCTURES) == 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_BONUSCHEST) != 0U &&
            (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_PVP) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS) == 0U &&
            (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_TNT) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_FIRESPREADS) == 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_HOSTFLY) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_HOSTHUNGER) == 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_BEDROCKFOG) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_DISABLESAVE) != 0U &&
            ((sessionConfig.hostSettings &
                    GAME_HOST_OPTION_BITMASK_WORLDSIZE) >>
                GAME_HOST_OPTION_BITMASK_WORLDSIZE_BITSHIFT) == 4U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_MOBGRIEFING) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_KEEPINVENTORY) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING) == 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_DOMOBLOOT) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_DOTILEDROPS) == 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_NATURALREGEN) != 0U &&
            (sessionConfig.hostSettings &
                GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE) == 0U &&
            sessionConfig.maxPlayers == 256 &&
            sessionConfig.networkMaxPlayers == 255U &&
            sessionConfig.hasSeed &&
            sessionConfig.seed == 4444 &&
            sessionConfig.worldSizeChunks == 320U &&
            sessionConfig.worldHellScale == 8U,
        sessionConfig.hostSettings,
        static_cast<unsigned int>(sessionConfig.networkMaxPlayers),
        static_cast<long long>(sessionConfig.seed),
        sessionConfig.worldSizeChunks,
        static_cast<unsigned int>(sessionConfig.worldHellScale),
        appSessionPlan.shouldInitGameSettings &&
            !appSessionPlan.tutorialMode &&
            !appSessionPlan.corruptSaveDeleted &&
            appSessionPlan.hostSettings == sessionConfig.hostSettings &&
            appSessionPlan.shouldApplyWorldSize &&
            appSessionPlan.worldSizeChunks == sessionConfig.worldSizeChunks &&
            appSessionPlan.worldHellScale == sessionConfig.worldHellScale &&
            appSessionPlan.saveDisabled == sessionConfig.saveDisabled);
    printf("app_session_runtime=%d applied=%d init=%d world_size=%d "
        "save_disabled=%d\n",
        appSessionRuntimeResult.applied &&
            appSessionRuntimeResult.initializedGameSettings &&
            appSessionRuntimeResult.appliedWorldSize &&
            appSessionRuntimeResult.saveDisabled ==
                appSessionPlan.saveDisabled,
        appSessionRuntimeResult.applied,
        appSessionRuntimeResult.initializedGameSettings,
        appSessionRuntimeResult.appliedWorldSize,
        appSessionRuntimeResult.saveDisabled);
    printf("world_bootstrap_plan=%d loaded_persist=%d created_new=%d "
        "failed=%d init_plan=%d init_seed=%lld init_players=%u "
        "hosted_plan=%d hosted_seed=%lld\n",
        loadedWorldPlan.targetWorldName == L"world" &&
            loadedWorldPlan.shouldPersistResolvedSaveId &&
            loadedWorldPlan.resolvedSaveId == "world_loaded" &&
            !loadedWorldPlan.createdNewWorld &&
            !loadedWorldPlan.loadFailed,
        loadedWorldLoadPlan.shouldPersistResolvedSaveId &&
            loadedWorldLoadPlan.resolvedSaveId == "world_loaded" &&
            !loadedWorldLoadPlan.shouldAbortStartup,
        createdWorldPlan.createdNewWorld,
        failedWorldLoadPlan.shouldAbortStartup &&
            failedWorldLoadPlan.abortExitCode == 4,
        networkInitPlan.seed == 7777 &&
            networkInitPlan.saveData == fakeSaveData &&
            networkInitPlan.settings == sessionConfig.hostSettings &&
            networkInitPlan.dedicatedNoLocalHostPlayer &&
            networkInitPlan.worldSizeChunks == sessionConfig.worldSizeChunks &&
            networkInitPlan.worldHellScale == sessionConfig.worldHellScale &&
            networkInitPlan.networkMaxPlayers ==
                sessionConfig.networkMaxPlayers,
        static_cast<long long>(networkInitPlan.seed),
        static_cast<unsigned int>(networkInitPlan.networkMaxPlayers),
        resolvedConfiguredSeed == sessionConfig.seed &&
            resolvedGeneratedSeed == 9999 &&
            hostedGamePlan.resolvedSeed == sessionConfig.seed &&
            hostedGamePlan.networkInitPlan.seed == sessionConfig.seed &&
            hostedGamePlan.networkInitPlan.saveData == fakeSaveData &&
            hostedGamePlan.networkInitPlan.settings ==
                sessionConfig.hostSettings &&
            hostedGamePlan.networkInitPlan.networkMaxPlayers ==
                sessionConfig.networkMaxPlayers &&
            hostedGamePlan.localUsersMask == 0 &&
            hostedGamePlan.onlineGame &&
            !hostedGamePlan.privateGame &&
            hostedGamePlan.publicSlots ==
                sessionConfig.networkMaxPlayers &&
            hostedGamePlan.privateSlots == 0 &&
            hostedGamePlan.fakeLocalPlayerJoined &&
            populatedInitData.seed == hostedGamePlan.networkInitPlan.seed &&
            populatedInitData.saveData == fakeSaveData &&
            populatedInitData.settings == sessionConfig.hostSettings &&
            populatedInitData.dedicatedNoLocalHostPlayer &&
            populatedInitData.xzSize == sessionConfig.worldSizeChunks &&
            populatedInitData.hellScale == sessionConfig.worldHellScale,
        static_cast<long long>(hostedGamePlan.resolvedSeed));
    printf("world_load_execution=%d dir=%d updated=%d saved=%d "
        "aborted=%d abort_code=%d\n",
        worldLoadExecutionDirectoryReady &&
            loadedWorldExecution.updatedSaveId &&
            loadedWorldExecution.savedResolvedSaveId &&
            !loadedWorldExecution.abortedStartup &&
            worldLoadExecutionText.find("level-id=world_loaded") !=
                std::string::npos &&
            failedWorldExecution.abortedStartup &&
            failedWorldExecution.abortExitCode == 4,
        worldLoadExecutionDirectoryReady,
        loadedWorldExecution.updatedSaveId,
        loadedWorldExecution.savedResolvedSaveId,
        failedWorldExecution.abortedStartup,
        failedWorldExecution.abortExitCode);
    printf("world_manager_native=%d dir=%d created_new=%d save_data=%d "
        "save_id=%s storage_root=%d\n",
        nativeWorldManagerDirectoryReady &&
            nativeWorldManagerBootstrap.status ==
                ServerRuntime::eWorldBootstrap_CreatedNew &&
            nativeWorldManagerBootstrap.saveData == nullptr &&
            nativeWorldManagerBootstrap.resolvedSaveId ==
                "NATIVE_SMOKE_WORLD" &&
            nativeWorldManagerStorageRootReady,
        nativeWorldManagerDirectoryReady,
        nativeWorldManagerBootstrap.status ==
            ServerRuntime::eWorldBootstrap_CreatedNew,
        nativeWorldManagerBootstrap.saveData == nullptr,
        nativeWorldManagerBootstrap.resolvedSaveId.c_str(),
        nativeWorldManagerStorageRootReady);
    printf("world_manager_native_loaded=%d dir=%d loaded=%d save_data=%d "
        "payload=%d save_id=%s storage_root=%d\n",
        nativeLoadedWorldManagerDirectoryReady &&
            nativeLoadedWorldManagerBootstrap.status ==
                ServerRuntime::eWorldBootstrap_Loaded &&
            nativeLoadedWorldManagerBootstrap.saveData == nullptr &&
            nativeLoadedWorldManagerPayloadCopied &&
            nativeLoadedWorldManagerBootstrap.resolvedSaveId ==
                "NATIVE_LOADED_WORLD" &&
            nativeLoadedWorldManagerStorageRootReady,
        nativeLoadedWorldManagerDirectoryReady,
        nativeLoadedWorldManagerBootstrap.status ==
            ServerRuntime::eWorldBootstrap_Loaded,
        nativeLoadedWorldManagerBootstrap.saveData == nullptr,
        nativeLoadedWorldManagerPayloadCopied,
        nativeLoadedWorldManagerBootstrap.resolvedSaveId.c_str(),
        nativeLoadedWorldManagerStorageRootReady);
    printf("world_manager_native_corrupt=%d dir=%d failed=%d save_data=%d "
        "save_id=%s storage_root=%d\n",
        nativeCorruptWorldManagerDirectoryReady &&
            nativeCorruptWorldManagerBootstrap.status ==
                ServerRuntime::eWorldBootstrap_Failed &&
            nativeCorruptWorldManagerBootstrap.saveData == nullptr &&
            nativeCorruptWorldManagerBootstrap.resolvedSaveId.empty() &&
            nativeCorruptWorldManagerStorageRootReady,
        nativeCorruptWorldManagerDirectoryReady,
        nativeCorruptWorldManagerBootstrap.status ==
            ServerRuntime::eWorldBootstrap_Failed,
        nativeCorruptWorldManagerBootstrap.saveData == nullptr,
        nativeCorruptWorldManagerBootstrap.resolvedSaveId.c_str(),
        nativeCorruptWorldManagerStorageRootReady);
    printf("startup_execution=%d dir=%d aborted=%d failed_abort=%d "
        "failed_code=%d\n",
        startupExecutionDirectoryReady &&
            !startupExecutionResult.abortedStartup &&
            startupExecutionResult.appSession.applied &&
            startupExecutionResult.worldLoadExecution.savedResolvedSaveId &&
            startupExecutionResult.hostedGameStartup.startupResult == 0 &&
            startupExecutionText.find("level-id=world_loaded") !=
                std::string::npos &&
            startupExecutionFailed.abortedStartup &&
            startupExecutionFailed.abortExitCode == 4,
        startupExecutionDirectoryReady,
        startupExecutionResult.abortedStartup,
        startupExecutionFailed.abortedStartup,
        startupExecutionFailed.abortExitCode);
    printf("autosave_state=%d initial_save=%d skip_initial_save=%d "
        "requested=%d next_advanced=%d completed=%d\n",
        autosaveState.intervalMs == 45000U &&
            autosaveState.nextTickMs == 91000U &&
            !autosaveState.autosaveRequested &&
            autosaveTriggerBefore &&
            autosaveTriggerAtDeadline &&
            autosaveCompletePlan.shouldMarkCompleted &&
            !autosaveCompletePlan.shouldRequestAutosave &&
            !autosaveCompletePlan.shouldAdvanceDeadline &&
            !autosaveRequestPlan.shouldMarkCompleted &&
            autosaveRequestPlan.shouldRequestAutosave &&
            !autosaveRequestPlan.shouldAdvanceDeadline &&
            !autosaveDelayPlan.shouldMarkCompleted &&
            !autosaveDelayPlan.shouldRequestAutosave &&
            autosaveDelayPlan.shouldAdvanceDeadline &&
            autosaveCatchupPlan.shouldMarkCompleted &&
            autosaveCatchupPlan.shouldRequestAutosave &&
            !autosaveCatchupPlan.shouldAdvanceDeadline,
        shouldRunInitialSave &&
            createdInitialSavePlan.shouldRequestInitialSave &&
            createdInitialSavePlan.idleWaitBeforeRequestMs == 5000 &&
            createdInitialSavePlan.requestTimeoutMs == 30000,
        shouldSkipInitialSaveWhenStopping,
        autosaveRequested,
        autosaveNextAdvanced,
        autosaveCompleted);
    printf("shutdown_plan=%d with_server_wait=%d without_server_wait=%d\n",
        shutdownPlanWithServer.shouldSetSaveOnExit &&
            shutdownPlanWithServer.shouldLogShutdownSave &&
            shutdownPlanWithServer.shouldWaitForServerStop &&
            !shutdownPlanWithoutServer.shouldSetSaveOnExit &&
            !shutdownPlanWithoutServer.shouldLogShutdownSave &&
            !shutdownPlanWithoutServer.shouldWaitForServerStop,
        shutdownPlanWithServer.shouldWaitForServerStop,
        shutdownPlanWithoutServer.shouldWaitForServerStop);
    printf("thread_startup_plan=%d ok_abort=%d failed_abort=%d failed_code=%d\n",
        !threadStartupPlanOk.shouldAbortStartup &&
            threadStartupPlanOk.abortExitCode == 0 &&
            threadStartupPlanFailed.shouldAbortStartup &&
            threadStartupPlanFailed.abortExitCode == 4,
        threadStartupPlanOk.shouldAbortStartup,
        threadStartupPlanFailed.shouldAbortStartup,
        threadStartupPlanFailed.abortExitCode);
    printf("shutdown_signal=%d raised=%d reset=%d\n",
        shutdownSignalInitiallyClear &&
            shutdownSignalRaised &&
            shutdownSignalReset,
        shutdownSignalRaised,
        shutdownSignalReset);
    printf("file_write=%d file_read=%d file_readback_match=%d utc_file_time=%llu\n",
        wroteSmokeFile,
        readSmokeFile,
        smokeFileReadback == smokeFileText,
        utcFileTime);
    printf("server_runtime_host=%s bind_ip=%s port=%d dedicated_port=%d "
        "host=%d join=%d dedicated=%d lan=%d default_autosave_ms=%llu "
        "configured_autosave_ms=%llu next_autosave_tick=%llu\n",
        runtimeState.hostNameUtf8.c_str(),
        runtimeState.bindIp.c_str(),
        runtimeState.multiplayerPort,
        runtimeState.dedicatedServerPort,
        runtimeState.multiplayerHost,
        runtimeState.multiplayerJoin,
        runtimeState.dedicatedServer,
        runtimeState.lanAdvertise,
        static_cast<unsigned long long>(defaultAutosaveMs),
        static_cast<unsigned long long>(configuredAutosaveMs),
        static_cast<unsigned long long>(nextAutosaveTick));
    printf("server_platform_state=%d utf8=%s players=%zu host_player=%d "
        "player1=%ls player2=%ls\n",
        platformState.userNameUtf8 == runtimeState.hostNameUtf8 &&
            platformState.userNameWide == runtimeState.hostNameWide &&
            platformState.bindIp == runtimeState.bindIp &&
            platformState.multiplayerPort == runtimeState.multiplayerPort &&
            platformState.dedicatedServerPort ==
                runtimeState.dedicatedServerPort &&
            platformState.multiplayerHost == runtimeState.multiplayerHost &&
            platformState.multiplayerJoin == runtimeState.multiplayerJoin &&
            platformState.dedicatedServer == runtimeState.dedicatedServer &&
            platformState.lanAdvertise == runtimeState.lanAdvertise &&
            platformState.players.size() == 3 &&
            platformState.players[0].smallId == 0U &&
            !platformState.players[0].isRemote &&
            platformState.players[0].isHostPlayer &&
            platformState.players[0].gamertag == L"RuntimeHost" &&
            platformState.players[1].smallId == 1U &&
            !platformState.players[1].isRemote &&
            !platformState.players[1].isHostPlayer &&
            platformState.players[1].gamertag == L"Player1" &&
            platformState.players[2].gamertag == L"Player2",
        platformState.userNameUtf8.c_str(),
        platformState.players.size(),
        platformState.players[0].isHostPlayer,
        platformState.players[1].gamertag.c_str(),
        platformState.players[2].gamertag.c_str());
    printf("platform_runtime=%d name=%s headless=%d\n",
        platformRuntimeResult.ok &&
            platformRuntimeResult.exitCode == 0 &&
            platformRuntimeResult.runtimeName == "NativeDesktopStub" &&
            platformRuntimeResult.headless,
        platformRuntimeResult.runtimeName.c_str(),
        platformRuntimeResult.headless);
    printf("run_hooks=%d bundle=%d\n",
        runHooks.afterStartupProc == &IncrementHookCount &&
            runHooks.afterStartupContext == &runAfterStartupCount &&
            runHooks.beforeSessionProc == &IncrementHookCount &&
            runHooks.beforeSessionContext == &runBeforeSessionCount &&
            runHooks.pollProc == &GameplayLoopRunPoll &&
            runHooks.pollContext == &runExecutionPollContext &&
            runHooks.afterSessionProc == &IncrementHookCount &&
            runHooks.afterSessionContext == &runAfterSessionCount,
        runHooks.afterStartupProc != nullptr &&
            runHooks.beforeSessionProc != nullptr &&
            runHooks.pollProc != nullptr &&
            runHooks.afterSessionProc != nullptr);
    printf("platform_actions=%d action_idle=%d pending=%d busy=%d "
        "wait_idle=%d\n",
        platformActionIdle &&
            platformActionPending &&
            platformWaitStillBusy &&
            platformWaitIdle,
        platformActionIdle,
        platformActionPending,
        platformWaitStillBusy,
        platformWaitIdle);
    printf("world_action_wait_hooks=%d idle=%d timeout=%d halt=%d "
        "ticks=%d handles=%d\n",
        worldActionWaitIdleOk &&
            waitHookIdleContext.tickCount == 2 &&
            waitHookIdleContext.handleCount == 2 &&
            worldActionWaitTimedOut &&
            waitHookTimeoutContext.tickCount == 1 &&
            waitHookTimeoutContext.handleCount == 1 &&
            worldActionWaitHalted &&
            waitHookHaltContext.tickCount == 0 &&
            waitHookHaltContext.handleCount == 0,
        worldActionWaitIdleOk,
        worldActionWaitTimedOut,
        worldActionWaitHalted,
        waitHookIdleContext.tickCount,
        waitHookIdleContext.handleCount);
    printf("platform_shutdown=%d gameplay=%d app_before=%d app_after=%d "
        "halt_before=%d halt_after=%d stop_valid=%d projected=%d\n",
        platformGameplayInstance &&
            platformAppShutdownBefore &&
            platformAppShutdownAfter &&
            platformGameplayHaltedBefore &&
            platformGameplayHaltedAfter &&
            platformStopSignalValid &&
            platformFlagsProjected,
        platformGameplayInstance,
        platformAppShutdownBefore,
        platformAppShutdownAfter,
        platformGameplayHaltedBefore,
        platformGameplayHaltedAfter,
        platformStopSignalValid,
        platformFlagsProjected);
    printf("platform_lifecycle=%d initial_requested=%d initial_completed=%d "
        "initial_timeout=%d shutdown_wait=%d shutdown_halt=%d\n",
        initialSaveExecution.requested &&
            initialSaveExecution.completed &&
            !initialSaveExecution.timedOut &&
            shutdownExecution.plan.shouldWaitForServerStop &&
            shutdownExecution.haltedGameplay,
        initialSaveExecution.requested,
        initialSaveExecution.completed,
        initialSaveExecution.timedOut,
        shutdownExecution.plan.shouldWaitForServerStop,
        shutdownExecution.haltedGameplay);
    printf("gameplay_loop=%d request=%d pending=%d complete=%d exit=%d\n",
        gameplayLoopRequest.autosaveRequested &&
            !gameplayLoopRequest.shouldExit &&
            !gameplayLoopPending.autosaveCompleted &&
            !gameplayLoopPending.autosaveRequested &&
            !gameplayLoopPending.shouldExit &&
            gameplayLoopComplete.autosaveCompleted &&
            !gameplayLoopComplete.shouldExit &&
            gameplayLoopExit.shouldExit,
        gameplayLoopRequest.autosaveRequested,
        !gameplayLoopPending.autosaveCompleted &&
            !gameplayLoopPending.autosaveRequested &&
            !gameplayLoopPending.shouldExit,
        gameplayLoopComplete.autosaveCompleted,
        gameplayLoopExit.shouldExit);
    printf("gameplay_loop_run=%d iterations=%zu polls=%d app_shutdown=%d\n",
        gameplayLoopRunResult.iterations == 3U &&
            gameplayLoopRunPollContext.pollCount == 3 &&
            gameplayLoopRunResult.requestedAppShutdown &&
            gameplayLoopRunResult.lastIteration.shouldExit,
        gameplayLoopRunResult.iterations,
        gameplayLoopRunPollContext.pollCount,
        gameplayLoopRunResult.requestedAppShutdown);
    printf("hosted_game_core_startup=%d exit=%d present=%d validated=%d "
        "startup=%llu/%llu phase=%s active=%d\n",
        nativeHostedCoreStartupResult.exitCode == 0 &&
            !nativeHostedCoreStartupResult.sessionSnapshot.loadedFromSave &&
            nativeHostedCoreStartupResult.sessionSnapshot.payloadValidated &&
            nativeHostedCoreStartupResult.sessionSnapshot
                    .startupThreadIterations == 2U &&
            nativeHostedCoreStartupResult.sessionSnapshot
                    .startupThreadDurationMs > 0U &&
            nativeHostedCoreStartupResult.sessionSnapshot.active &&
            nativeHostedCoreStartupResult.sessionSnapshot.hostedThreadActive ==
                false &&
            nativeHostedCoreStartupResult.sessionSnapshot.runtimePhase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Startup,
        nativeHostedCoreStartupResult.exitCode,
        nativeHostedCoreStartupResult.sessionSnapshot.loadedFromSave,
        nativeHostedCoreStartupResult.sessionSnapshot.payloadValidated,
        (unsigned long long)nativeHostedCoreStartupResult.sessionSnapshot
            .startupThreadIterations,
        (unsigned long long)nativeHostedCoreStartupResult.sessionSnapshot
            .startupThreadDurationMs,
        ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
            (ServerRuntime::EDedicatedServerHostedGameRuntimePhase)
                nativeHostedCoreStartupResult.sessionSnapshot.runtimePhase),
        nativeHostedCoreStartupResult.sessionSnapshot.active);
    printf("hosted_game_core_frame=%d start=%d first=%llu/%llu/%d second=%llu/%llu/%d stopped=%d\n",
            nativeHostedCoreFrameStarted &&
            nativeHostedCoreFrameFirst.frameTimestampMs > 0U &&
            nativeHostedCoreFrameFirst.workerFrame.autosaveCompletions == 0U &&
            nativeHostedCoreFrameFirst.workerFrame.nextSleepDurationMs == 10U &&
            !nativeHostedCoreFrameFirst.workerFrame.shutdownRequested &&
            !nativeHostedCoreFrameFirst.workerFrame.shouldStopRunning &&
            nativeHostedCoreFrameFirst.workerFrame.snapshot
                    .pendingWorldActionTicks == 1U &&
            nativeHostedCoreFrameFirst.workerFrame.snapshot
                    .processedAutosaveCommands == 0U &&
            !nativeHostedCoreFrameFirst.workerFrame.idle &&
            nativeHostedCoreFrameFirst.sessionSnapshot.sessionTicks == 1U &&
            nativeHostedCoreFrameFirst.sessionSnapshot
                    .gameplayLoopIterations == 1U &&
            nativeHostedCoreFrameFirst.sessionSnapshot.hostedThreadActive &&
            nativeHostedCoreFrameFirst.sessionSnapshot.runtimePhase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Running &&
            nativeHostedCoreFrameSecond.frameTimestampMs >=
                nativeHostedCoreFrameFirst.frameTimestampMs &&
            nativeHostedCoreFrameSecond.workerFrame.autosaveCompletions ==
                1U &&
            nativeHostedCoreFrameSecond.workerFrame.nextSleepDurationMs == 10U &&
            !nativeHostedCoreFrameSecond.workerFrame.shutdownRequested &&
            !nativeHostedCoreFrameSecond.workerFrame.shouldStopRunning &&
            nativeHostedCoreFrameSecond.workerFrame.snapshot
                    .pendingWorldActionTicks == 0U &&
            nativeHostedCoreFrameSecond.workerFrame.snapshot
                    .processedAutosaveCommands == 1U &&
            nativeHostedCoreFrameSecond.workerFrame.idle &&
            nativeHostedCoreFrameSecond.sessionSnapshot.sessionTicks == 2U &&
            nativeHostedCoreFrameSecond.sessionSnapshot
                    .gameplayLoopIterations == 2U &&
            nativeHostedCoreFrameSecond.sessionSnapshot
                    .observedAutosaveCompletions == 1U &&
            nativeHostedCoreFrameSecond.sessionSnapshot.hostedThreadActive &&
            nativeHostedCoreFrameSecond.sessionSnapshot.runtimePhase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Running &&
            !nativeHostedCoreFrameStoppedSnapshot.active &&
            nativeHostedCoreFrameStoppedSnapshot.runtimePhase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Stopped,
        nativeHostedCoreFrameStarted,
        (unsigned long long)nativeHostedCoreFrameFirst
            .workerFrame.snapshot.pendingWorldActionTicks,
        (unsigned long long)nativeHostedCoreFrameFirst
            .workerFrame.autosaveCompletions,
        nativeHostedCoreFrameFirst.workerFrame.idle,
        (unsigned long long)nativeHostedCoreFrameSecond
            .workerFrame.snapshot.pendingWorldActionTicks,
        (unsigned long long)nativeHostedCoreFrameSecond
            .workerFrame.autosaveCompletions,
        nativeHostedCoreFrameSecond.workerFrame.idle,
        !nativeHostedCoreFrameStoppedSnapshot.active);
    printf("hosted_game_core=%d exit=%d validated=%d startup=%llu/%llu "
        "loops=%llu autosaves=%llu worker_idle=%d hooks=%d/%d phase=%s\n",
        nativeHostedCoreRunResult.exitCode == 0 &&
            nativeHostedCoreRunResult.startup.sessionSnapshot
                    .payloadValidated &&
            nativeHostedCoreRunResult.startup.sessionSnapshot
                    .startupThreadIterations == 2U &&
            nativeHostedCoreRunResult.startup.sessionSnapshot
                    .startupThreadDurationMs > 0U &&
            nativeHostedCoreRunResult.loopIterations == 1U &&
            nativeHostedCoreRunResult.finalState.autosaveCompletions == 0U &&
            nativeHostedCoreRunResult.lastFrame.frameTimestampMs > 0U &&
            nativeHostedCoreRunResult.lastFrame.workerFrame
                    .nextSleepDurationMs == 0U &&
            nativeHostedCoreRunResult.lastFrame.workerFrame
                    .shouldStopRunning &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingWorldActionTicks == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingAutosaveCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingSaveCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingStopCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingHaltCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .activeCommandKind ==
                ServerRuntime::
                    eNativeDedicatedServerHostedGameWorkerCommand_None &&
            g_nativeHostedCoreHookSmokeContext.readyCount == 1 &&
            g_nativeHostedCoreHookSmokeContext.stoppedCount == 1 &&
            !nativeHostedCoreSnapshot.active &&
            nativeHostedCoreSnapshot.runtimePhase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Stopped,
        nativeHostedCoreRunResult.exitCode,
        nativeHostedCoreRunResult.startup.sessionSnapshot.payloadValidated,
        (unsigned long long)nativeHostedCoreRunResult.startup
            .sessionSnapshot.startupThreadIterations,
        (unsigned long long)nativeHostedCoreRunResult.startup
            .sessionSnapshot.startupThreadDurationMs,
        (unsigned long long)nativeHostedCoreRunResult.loopIterations,
        (unsigned long long)nativeHostedCoreRunResult.finalState
            .autosaveCompletions,
        nativeHostedCoreRunResult.finalState.workerSnapshot
            .activeCommandKind ==
            ServerRuntime::
                eNativeDedicatedServerHostedGameWorkerCommand_None,
        g_nativeHostedCoreHookSmokeContext.readyCount,
        g_nativeHostedCoreHookSmokeContext.stoppedCount,
        ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
            (ServerRuntime::EDedicatedServerHostedGameRuntimePhase)
                nativeHostedCoreSnapshot.runtimePhase));
    printf("hosted_game_runtime=%d result=%d thread_value=%d\n",
        restartedHostedRuntimeResult.ok &&
            nativeHostedCoreRunResult.exitCode == 0 &&
            nativeHostedCoreRunResult.startup.sessionSnapshot
                    .payloadValidated &&
            nativeHostedCoreRunResult.startup.sessionSnapshot
                    .startupThreadIterations == 2U &&
            nativeHostedCoreRunResult.startup.sessionSnapshot
                    .startupThreadDurationMs > 0U &&
            nativeHostedCoreRunResult.loopIterations == 1U &&
            nativeHostedCoreRunResult.finalState.autosaveCompletions == 0U &&
            nativeHostedCoreRunResult.lastFrame.frameTimestampMs > 0U &&
            nativeHostedCoreRunResult.lastFrame.workerFrame
                    .nextSleepDurationMs == 0U &&
            nativeHostedCoreRunResult.lastFrame.workerFrame
                    .shouldStopRunning &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingWorldActionTicks == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingAutosaveCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingSaveCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingStopCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .pendingHaltCommands == 0U &&
            nativeHostedCoreRunResult.finalState.workerSnapshot
                    .activeCommandKind ==
                ServerRuntime::
                    eNativeDedicatedServerHostedGameWorkerCommand_None &&
            g_nativeHostedCoreHookSmokeContext.readyCount == 1 &&
            g_nativeHostedCoreHookSmokeContext.stoppedCount == 1 &&
            !nativeHostedCoreSnapshot.active &&
            nativeHostedCoreSnapshot.runtimePhase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Stopped &&
            hostedGameRuntimeSleepResult == 13 &&
            hostedGameRuntimeSleepThreadValue == 2 &&
            hostedGameRuntimeResult == 11 &&
            hostedGameRuntimeThreadValue == 1 &&
            hostedGameRuntimeNullThreadResult == -1 &&
            hostedGameRuntimeNullThreadSnapshot.startAttempted &&
            !hostedGameRuntimeNullThreadSnapshot.threadInvoked &&
            hostedGameRuntimeNullThreadSnapshot.startupResult == -1 &&
            hostedGameRuntimeNullThreadSnapshot.phase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Failed,
        hostedGameRuntimeResult,
        hostedGameRuntimeThreadValue);
    const bool nativeHostedStubCoreContextOk =
        nativeHostedSessionCoreSnapshot.startAttempted &&
        nativeHostedSessionCoreSnapshot.loadedFromSave &&
        nativeHostedSessionCoreSnapshot.resolvedSeed ==
            hostedGamePlan.resolvedSeed &&
        nativeHostedSessionCoreSnapshot.hostSettings ==
            sessionConfig.hostSettings &&
        nativeHostedSessionCoreSnapshot.dedicatedNoLocalHostPlayer &&
        nativeHostedSessionCoreSnapshot.worldSizeChunks ==
            sessionConfig.worldSizeChunks &&
        nativeHostedSessionCoreSnapshot.worldHellScale ==
            sessionConfig.worldHellScale &&
        nativeHostedSessionCoreSnapshot.savePayloadBytes ==
            nativeHostedSaveBytes.size() &&
        nativeHostedSessionCoreSnapshot.savePayloadName == "world" &&
        nativeHostedSessionCoreSnapshot.loadedSaveMetadataAvailable &&
        nativeHostedSessionCoreSnapshot.loadedSavePath ==
            "NativeDesktop/GameHDD/SmokeSession.save" &&
        nativeHostedSessionCoreSnapshot.previousStartupMode == "loaded";
    const bool nativeHostedStubCoreFinalizationOk =
        nativeHostedSessionCoreStoppedSnapshot.saveGeneration >=
            nativeHostedSessionCoreSnapshot.saveGeneration &&
        nativeHostedSessionCoreStoppedSnapshot.stateChecksum != 0U &&
        !nativeHostedSessionCoreStoppedSnapshot.active;
    const bool nativeHostedStubCoreOk =
        nativeHostedStubCoreContextOk &&
        nativeHostedStubCoreFinalizationOk;
    const bool nativeHostedStubRuntimeStartupOk =
        nativeHostedStubSnapshot.startAttempted &&
        nativeHostedStubSnapshot.threadInvoked &&
        nativeHostedStubSnapshot.startupResult == 0 &&
        nativeHostedStubSnapshot.startupPayloadPresent &&
        nativeHostedStubSnapshot.startupPayloadValidated &&
        nativeHostedStubSnapshot.startupThreadIterations == 4U &&
        nativeHostedStubSnapshot.startupThreadDurationMs > 0U &&
        nativeHostedStubSnapshot.hostedThreadActive &&
        nativeHostedStubSnapshot.loadedSaveMetadataAvailable &&
        nativeHostedStubSnapshot.previousStartupMode == "loaded";
    const bool nativeHostedStubRuntimeStopOk =
        nativeHostedStubStopped &&
        nativeHostedStubHaltQueued &&
        nativeHostedStubExitCode == 0 &&
        nativeHostedStubThreadTicks > 0U &&
        !nativeHostedStubStoppedSnapshot.hostedThreadActive &&
        nativeHostedStubStoppedSnapshot.hostedThreadTicks >=
            nativeHostedStubThreadTicks;
    const bool nativeHostedStubRuntimeOk =
        nativeHostedStubRuntimeStartupOk &&
        nativeHostedStubRuntimeStopOk;
    printf("native_hosted_stub=%d result=%d steps=%llu duration-ms=%llu "
        "ticks=%llu validated=%d halt=%d stopped=%d exit=%lu thread-ticks=%llu "
        "core-generation=%llu/%llu core-checksum=0x%016llx "
        "worker=%llu/%llu/%llu/%llu/%llu/%llu\n",
        nativeHostedStubResult == 0 &&
            nativeHostedStubInitData.seed == hostedGamePlan.resolvedSeed &&
            nativeHostedSaveTextBuilt &&
            nativeHostedStubInitData.saveData == nativeHostedSaveData &&
            nativeHostedStubInitData.settings == sessionConfig.hostSettings &&
            nativeHostedStubInitData.localUsersMask ==
                hostedGamePlan.localUsersMask &&
            nativeHostedStubInitData.onlineGame ==
                hostedGamePlan.onlineGame &&
            nativeHostedStubInitData.privateGame ==
                hostedGamePlan.privateGame &&
            nativeHostedStubInitData.publicSlots ==
                hostedGamePlan.publicSlots &&
            nativeHostedStubInitData.privateSlots ==
                hostedGamePlan.privateSlots &&
            nativeHostedStubInitData.fakeLocalPlayerJoined ==
                hostedGamePlan.fakeLocalPlayerJoined &&
            nativeHostedStubInitData.dedicatedNoLocalHostPlayer &&
            nativeHostedStubInitData.xzSize == sessionConfig.worldSizeChunks &&
            nativeHostedStubInitData.hellScale ==
                sessionConfig.worldHellScale &&
            nativeHostedStubTickAfter > nativeHostedStubTickBefore &&
            nativeHostedStubCoreOk &&
            nativeHostedStubRuntimeOk,
        nativeHostedStubResult,
        (unsigned long long)
            nativeHostedStubSnapshot.startupThreadIterations,
        (unsigned long long)
            nativeHostedStubSnapshot.startupThreadDurationMs,
        (unsigned long long)
            (nativeHostedStubTickAfter - nativeHostedStubTickBefore),
        nativeHostedStubSnapshot.startupPayloadValidated,
        nativeHostedStubHaltQueued,
        nativeHostedStubStopped,
        (unsigned long)nativeHostedStubExitCode,
        (unsigned long long)nativeHostedStubThreadTicks,
        (unsigned long long)nativeHostedSessionCoreSnapshot.saveGeneration,
        (unsigned long long)nativeHostedSessionCoreStoppedSnapshot
            .saveGeneration,
        (unsigned long long)nativeHostedSessionCoreStoppedSnapshot
            .stateChecksum,
        (unsigned long long)
            nativeHostedSessionCoreSnapshot.workerPendingSaveCommands,
        (unsigned long long)
            nativeHostedSessionCoreSnapshot.workerPendingStopCommands,
        (unsigned long long)
            nativeHostedSessionCoreSnapshot.workerPendingHaltCommands,
        (unsigned long long)
            nativeHostedSessionCoreSnapshot.processedSaveCommands,
        (unsigned long long)
            nativeHostedSessionCoreSnapshot.processedStopCommands,
        (unsigned long long)
            nativeHostedSessionCoreSnapshot.processedHaltCommands);
    printf("native_hosted_stub_contract=%d core=%d/%d runtime=%d/%d\n",
        nativeHostedStubCoreOk && nativeHostedStubRuntimeOk,
        nativeHostedStubCoreContextOk,
        nativeHostedStubCoreFinalizationOk,
        nativeHostedStubRuntimeStartupOk,
        nativeHostedStubRuntimeStopOk);
    printf("native_hosted_stub_core_context loaded=%d start=%d seed=%lld "
        "host=0x%x nolocal=%d size=%u hell=%u payload=%s/%lld meta=%d "
        "path=%s mode=%s\n",
        nativeHostedSessionCoreSnapshot.loadedFromSave,
        nativeHostedSessionCoreSnapshot.startAttempted,
        (long long)nativeHostedSessionCoreSnapshot.resolvedSeed,
        nativeHostedSessionCoreSnapshot.hostSettings,
        nativeHostedSessionCoreSnapshot.dedicatedNoLocalHostPlayer,
        nativeHostedSessionCoreSnapshot.worldSizeChunks,
        nativeHostedSessionCoreSnapshot.worldHellScale,
        nativeHostedSessionCoreSnapshot.savePayloadName.c_str(),
        (long long)nativeHostedSessionCoreSnapshot.savePayloadBytes,
        nativeHostedSessionCoreSnapshot.loadedSaveMetadataAvailable,
        nativeHostedSessionCoreSnapshot.loadedSavePath.c_str(),
        nativeHostedSessionCoreSnapshot.previousStartupMode.c_str());
    printf("hosted_game_startup=%d result=%d abort=%d code=%d\n",
        hostedGameStartupExecution.startupResult == 0 &&
            !hostedGameStartupExecution.startupPlan.shouldAbortStartup &&
            hostedGameStartupExecution.startupPlan.abortExitCode == 0 &&
            hostedGameStartupInitData.seed == hostedGamePlan.resolvedSeed &&
            hostedGameStartupInitData.saveData == fakeSaveData &&
            hostedGameStartupInitData.settings ==
                sessionConfig.hostSettings &&
            hostedGameStartupInitData.dedicatedNoLocalHostPlayer &&
            hostedGameStartupInitData.xzSize == sessionConfig.worldSizeChunks &&
            hostedGameStartupInitData.hellScale ==
                sessionConfig.worldHellScale,
        hostedGameStartupExecution.startupResult,
        hostedGameStartupExecution.startupPlan.shouldAbortStartup,
        hostedGameStartupExecution.startupPlan.abortExitCode);
    printf("hosted_game_snapshot=%d loaded=%d seed=%lld public=%u "
        "startup=%d thread=%d phase=%s "
        "prev-worker=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
        "prev-active=%s#%llu/%llu\n",
        hostedGameRuntimeSnapshot.startAttempted &&
            hostedGameRuntimeSnapshot.threadInvoked &&
            hostedGameRuntimeSnapshot.loadedFromSave &&
            hostedGameRuntimeSnapshot.resolvedSeed ==
                hostedGamePlan.resolvedSeed &&
            hostedGameRuntimeSnapshot.worldSizeChunks ==
                sessionConfig.worldSizeChunks &&
            hostedGameRuntimeSnapshot.worldHellScale ==
                sessionConfig.worldHellScale &&
            hostedGameRuntimeSnapshot.publicSlots ==
                sessionConfig.networkMaxPlayers &&
            hostedGameRuntimeSnapshot.privateSlots == 0 &&
            hostedGameRuntimeSnapshot.onlineGame &&
            !hostedGameRuntimeSnapshot.privateGame &&
            hostedGameRuntimeSnapshot.fakeLocalPlayerJoined &&
            hostedGameRuntimeSnapshot.loadedSaveMetadataAvailable &&
            hostedGameRuntimeSnapshot.loadedSavePath ==
                "NativeDesktop/GameHDD/SmokeSession.save" &&
            hostedGameRuntimeSnapshot.previousStartupMode == "loaded" &&
            hostedGameRuntimeSnapshot.previousRemoteCommands == 9 &&
            hostedGameRuntimeSnapshot.previousAutosaveCompletions == 7 &&
            hostedGameRuntimeSnapshot
                .previousWorkerPendingWorldActionTicks == 1 &&
            hostedGameRuntimeSnapshot.previousWorkerPendingAutosaveCommands == 4 &&
            hostedGameRuntimeSnapshot.previousWorkerPendingSaveCommands == 2 &&
            hostedGameRuntimeSnapshot.previousWorkerPendingStopCommands == 3 &&
            hostedGameRuntimeSnapshot.previousWorkerPendingHaltCommands == 1 &&
            hostedGameRuntimeSnapshot.previousWorkerTickCount == 22 &&
            hostedGameRuntimeSnapshot.previousCompletedWorkerActions == 6 &&
            hostedGameRuntimeSnapshot.previousProcessedAutosaveCommands == 7 &&
            hostedGameRuntimeSnapshot.previousProcessedSaveCommands == 13 &&
            hostedGameRuntimeSnapshot.previousProcessedStopCommands == 5 &&
            hostedGameRuntimeSnapshot.previousProcessedHaltCommands == 6 &&
            hostedGameRuntimeSnapshot.previousLastQueuedCommandId == 17 &&
            hostedGameRuntimeSnapshot.previousActiveCommandId == 15 &&
            hostedGameRuntimeSnapshot.previousActiveCommandTicksRemaining == 2 &&
            hostedGameRuntimeSnapshot.previousActiveCommandKind ==
                ServerRuntime::
                    eNativeDedicatedServerHostedGameWorkerCommand_Autosave &&
            hostedGameRuntimeSnapshot.previousLastProcessedCommandId == 16 &&
            hostedGameRuntimeSnapshot.previousLastProcessedCommandKind ==
                ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_Stop &&
            hostedGameRuntimeSnapshot.previousPlatformTickCount == 42 &&
            hostedGameRuntimeSnapshot.previousUptimeMs == 1337 &&
            hostedGameRuntimeSnapshot.previousHostSettings == 0xCAFEU &&
            !hostedGameRuntimeSnapshot
                .previousDedicatedNoLocalHostPlayer &&
            hostedGameRuntimeSnapshot.previousWorldSizeChunks == 160U &&
            hostedGameRuntimeSnapshot.previousWorldHellScale == 3U &&
            hostedGameRuntimeSnapshot.previousOnlineGame &&
            !hostedGameRuntimeSnapshot.previousPrivateGame &&
            hostedGameRuntimeSnapshot.previousFakeLocalPlayerJoined &&
            hostedGameRuntimeSnapshot.previousPublicSlots == 16 &&
            hostedGameRuntimeSnapshot.previousPrivateSlots == 1 &&
            hostedGameRuntimeSnapshot.previousSavePayloadChecksum ==
                0x123456789abcdef0ULL &&
            hostedGameRuntimeSnapshot.previousSaveGeneration == 11U &&
            hostedGameRuntimeSnapshot.previousSessionStateChecksum ==
                0x0fedcba987654321ULL &&
            hostedGameRuntimeSnapshot.savePayloadChecksum ==
                fakeSaveChecksum &&
            hostedGameRuntimeSnapshot.hostSettings ==
                sessionConfig.hostSettings &&
            hostedGameRuntimeSnapshot.dedicatedNoLocalHostPlayer &&
            hostedGameRuntimeSnapshot.phase ==
                ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Startup &&
            hostedGameRuntimeSnapshot.startupResult == 0,
        hostedGameRuntimeSnapshot.loadedFromSave,
        (long long)hostedGameRuntimeSnapshot.resolvedSeed,
        (unsigned int)hostedGameRuntimeSnapshot.publicSlots,
        hostedGameRuntimeSnapshot.startupResult,
        hostedGameRuntimeSnapshot.threadInvoked,
        ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
            hostedGameRuntimeSnapshot.phase),
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousWorkerPendingWorldActionTicks,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousWorkerPendingAutosaveCommands,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousWorkerPendingSaveCommands,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousWorkerPendingStopCommands,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousWorkerPendingHaltCommands,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousWorkerTickCount,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousCompletedWorkerActions,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousProcessedAutosaveCommands,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousProcessedSaveCommands,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousProcessedStopCommands,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousProcessedHaltCommands,
        ServerRuntime::GetNativeDedicatedServerHostedGameWorkerCommandKindName(
            (ServerRuntime::ENativeDedicatedServerHostedGameWorkerCommandKind)
                hostedGameRuntimeSnapshot.previousActiveCommandKind),
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousActiveCommandId,
        (unsigned long long)
            hostedGameRuntimeSnapshot.previousActiveCommandTicksRemaining);
    ServerRuntime::ResetNativeDedicatedServerLoadedSaveMetadata();
    printf("hosted_game_session=%d active=%d stopped=%d payload=%s bytes=%lld "
        "ticks=%llu action=%s shutdown=%d halted=%d uptime=%llu "
        "autosaves=%llu/%llu "
        "worker=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
        "active=%s#%llu/%llu "
        "remote=%llu accepted=%llu phase=%s save=%s persisted=%s "
        "filetime=%llu loop=%llu completed=%d "
        "observed-worker=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
        "observed-active=%s#%llu/%llu "
        "stopped-phase=%s\n",
        hostedGameSessionOk,
        hostedGameSessionSnapshot.sessionActive,
        hostedGameStoppedSnapshot.stoppedMs == 1200,
        hostedGameSessionSnapshot.savePayloadName.c_str(),
        (long long)hostedGameSessionSnapshot.savePayloadBytes,
        (unsigned long long)hostedGameSessionSnapshot.platformTickCount,
        hostedGameSessionSnapshot.worldActionIdle ? "idle" : "busy",
        hostedGameSessionSnapshot.appShutdownRequested,
        hostedGameSessionSnapshot.gameplayHalted,
        (unsigned long long)hostedGameSessionSnapshot.uptimeMs,
        (unsigned long long)hostedGameSessionSnapshot.autosaveRequests,
        (unsigned long long)hostedGameSessionSnapshot.autosaveCompletions,
        (unsigned long long)
            hostedGameSessionSnapshot.workerPendingWorldActionTicks,
        (unsigned long long)
            hostedGameSessionSnapshot.workerPendingAutosaveCommands,
        (unsigned long long)
            hostedGameSessionSnapshot.workerPendingSaveCommands,
        (unsigned long long)
            hostedGameSessionSnapshot.workerPendingStopCommands,
        (unsigned long long)
            hostedGameSessionSnapshot.workerPendingHaltCommands,
        (unsigned long long)hostedGameSessionSnapshot.workerTickCount,
        (unsigned long long)
            hostedGameSessionSnapshot.completedWorkerActions,
        (unsigned long long)
            hostedGameSessionSnapshot.processedAutosaveCommands,
        (unsigned long long)
            hostedGameSessionSnapshot.processedSaveCommands,
        (unsigned long long)
            hostedGameSessionSnapshot.processedStopCommands,
        (unsigned long long)
            hostedGameSessionSnapshot.processedHaltCommands,
        ServerRuntime::GetNativeDedicatedServerHostedGameWorkerCommandKindName(
            (ServerRuntime::ENativeDedicatedServerHostedGameWorkerCommandKind)
                hostedGameSessionSnapshot.activeCommandKind),
        (unsigned long long)
            hostedGameSessionSnapshot.activeCommandId,
        (unsigned long long)
            hostedGameSessionSnapshot.activeCommandTicksRemaining,
        (unsigned long long)hostedGameSessionSnapshot.remoteCommands,
        (unsigned long long)hostedGameSessionSnapshot.acceptedConnections,
        ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
            hostedGameSessionSnapshot.phase),
        hostedGameSessionSnapshot.savePath.c_str(),
        hostedGameSessionSnapshot.lastPersistedSavePath.c_str(),
        (unsigned long long)hostedGameSessionSnapshot.lastPersistedFileTime,
        (unsigned long long)hostedGameSessionSnapshot.gameplayLoopIterations,
        hostedGameSessionSnapshot.sessionCompleted,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.workerPendingWorldActionTicks,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.workerPendingAutosaveCommands,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.workerPendingSaveCommands,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.workerPendingStopCommands,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.workerPendingHaltCommands,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.workerTickCount,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.completedWorkerActions,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.processedAutosaveCommands,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.processedSaveCommands,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.processedStopCommands,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.processedHaltCommands,
        ServerRuntime::GetNativeDedicatedServerHostedGameWorkerCommandKindName(
            nativeHostedSessionObservedSnapshot.activeCommandKind),
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.activeCommandId,
        (unsigned long long)
            nativeHostedSessionObservedSnapshot.activeCommandTicksRemaining,
        ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
            hostedGameStoppedSnapshot.phase));
    printf("hosted_game_session_contract=%d current=%d observed=%d/%d/%d "
        "stopped=%d\n",
        hostedGameSessionOk,
        hostedGameSessionCurrentSnapshotOk,
        hostedGameSessionObservedIdentityOk,
        hostedGameSessionObservedWorkerOk,
        hostedGameSessionObservedLifecycleOk,
        hostedGameSessionStoppedOk);
    printf("hosted_game_session_lifecycle shutdown=%d stop=%d initial=%d/%d/%d "
        "completed=%d requested=%d halted=%d phase=%s stopped_ms=%llu "
        "checksum=0x%016llx\n",
        nativeHostedSessionObservedSnapshot.appShutdownRequested,
        nativeHostedSessionObservedSnapshot.stopSignalValid,
        nativeHostedSessionObservedSnapshot.initialSaveRequested,
        nativeHostedSessionObservedSnapshot.initialSaveCompleted,
        nativeHostedSessionObservedSnapshot.initialSaveTimedOut,
        nativeHostedSessionObservedSnapshot.sessionCompleted,
        nativeHostedSessionObservedSnapshot.requestedAppShutdown,
        nativeHostedSessionObservedSnapshot.shutdownHaltedGameplay,
        ServerRuntime::GetDedicatedServerHostedGameRuntimePhaseName(
            (ServerRuntime::EDedicatedServerHostedGameRuntimePhase)
                nativeHostedSessionObservedSnapshot.runtimePhase),
        (unsigned long long)nativeHostedSessionObservedSnapshot.stoppedMs,
        (unsigned long long)nativeHostedSessionObservedSnapshot.stateChecksum);
    printf("session_execution=%d runtime=%d initial=%d shutdown=%d "
        "iterations=%zu polls=%d\n",
        platformSessionRuntimeResult.ok &&
            sessionExecution.initialSave.requested &&
            sessionExecution.initialSave.completed &&
            !sessionExecution.initialSave.timedOut &&
            sessionExecution.autosaveState.intervalMs == 45000U &&
            sessionExecution.gameplayLoop.iterations == 3U &&
            sessionExecution.gameplayLoop.requestedAppShutdown &&
            sessionExecution.gameplayLoop.lastIteration.shouldExit &&
            sessionExecution.shutdown.haltedGameplay &&
            sessionExecution.shutdown.plan.shouldSetSaveOnExit &&
            sessionExecution.shutdown.plan.shouldWaitForServerStop &&
            sessionGameplayHalted &&
            sessionPollContext.pollCount == 3,
        platformSessionRuntimeResult.ok,
        sessionExecution.initialSave.completed,
        sessionExecution.shutdown.haltedGameplay,
        sessionExecution.gameplayLoop.iterations,
        sessionPollContext.pollCount);
    printf("run_execution=%d runtime=%d completed=%d startup=%d "
        "shutdown=%d hooks=%d/%d/%d polls=%d\n",
        platformRunRuntimeResult.ok &&
            !runExecution.abortedStartup &&
            runExecution.completedSession &&
            runExecution.startup.appSession.applied &&
            runExecution.session.initialSave.requested &&
            runExecution.session.initialSave.completed &&
            runExecution.session.gameplayLoop.iterations == 3U &&
            runExecution.session.shutdown.haltedGameplay &&
            runExecutionGameplayHalted &&
            runAfterStartupCount == 1 &&
            runBeforeSessionCount == 1 &&
            runAfterSessionCount == 1 &&
            runExecutionPollContext.pollCount == 3,
        platformRunRuntimeResult.ok,
        runExecution.completedSession,
        runExecution.startup.appSession.applied,
        runExecution.session.shutdown.haltedGameplay,
        runAfterStartupCount,
        runBeforeSessionCount,
        runAfterSessionCount,
        runExecutionPollContext.pollCount);
    printf("autosave_tracker=%d requests=%llu completions=%llu "
        "pending_before=%d pending_after=%d\n",
        autosaveTrackerPendingSnapshot.autosavePending &&
            !autosaveTrackerPendingSnapshot.lastObservedIdle &&
            autosaveTrackerPendingSnapshot.requestCount == 2 &&
            autosaveTrackerPendingSnapshot.completionCount == 0 &&
            !autosaveTrackerCompletedSnapshot.autosavePending &&
            autosaveTrackerCompletedSnapshot.lastObservedIdle &&
            autosaveTrackerCompletedSnapshot.requestCount == 2 &&
            autosaveTrackerCompletedSnapshot.completionCount == 1 &&
            platformAutosaveRequestsAfterWait == 1 &&
            platformAutosaveCompletionsAfterWait == 1,
        (unsigned long long)autosaveTrackerCompletedSnapshot.requestCount,
        (unsigned long long)autosaveTrackerCompletedSnapshot.completionCount,
        autosaveTrackerPendingSnapshot.autosavePending,
        autosaveTrackerCompletedSnapshot.autosavePending);
    printf("server_storage_platform=%s game_hdd_root=%s override_root=%s\n",
        storagePlatformDirectory,
        storageGameHddRoot.c_str(),
        overriddenStorageRoot.c_str());
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
    printf("bootstrap_prepare=%d bootstrap_environment=%d access_ready=%d "
        "access_shutdown=%d restored_dir=%d runtime_host=%s bind_ip=%s "
        "port=%d storage_root=%s world=%ls level_id=%s\n",
        bootstrapPrepared,
        bootstrapEnvironmentReady,
        bootstrapAccessReady,
        bootstrapAccessShutdown,
        bootstrapRestoredDirectory,
        bootstrapSmokeContext.runtimeState.hostNameUtf8.c_str(),
        bootstrapSmokeContext.runtimeState.bindIp.c_str(),
        bootstrapSmokeContext.runtimeState.multiplayerPort,
        bootstrapSmokeContext.storageRoot.c_str(),
        bootstrapSmokeContext.worldTarget.worldName.c_str(),
        bootstrapSmokeContext.worldTarget.saveId.c_str());
    printf("socket_bootstrap_started=%d connected=%d accepted=%d stopped=%d "
        "bound_port=%d accepted_ip=%s accepted_port=%d\n",
        socketBootstrapStarted,
        socketBootstrapConnected,
        socketBootstrapAcceptedOk,
        socketBootstrapStopped,
        socketBootstrapStarted ? socketBootstrapBoundPort : -1,
        socketBootstrapAcceptedIp,
        socketBootstrapAcceptedPort);
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
    printf("world_file_header_duplicate=%d round_trip=%d prefixes=%d "
        "offsets=%d file_count=%d total_size=%u\n",
        worldFileHeaderSmoke.duplicateReused,
        worldFileHeaderSmoke.roundTripOk,
        worldFileHeaderSmoke.prefixesOk,
        worldFileHeaderSmoke.offsetsOk,
        worldFileHeaderSmoke.fileCount,
        worldFileHeaderSmoke.totalSize);
    printf("server_properties_defaults=%d normalized=%d persisted=%d "
        "whitelist=%d\n",
        serverPropertiesSmoke.createdDefaults,
        serverPropertiesSmoke.normalizedValues,
        serverPropertiesSmoke.savePersisted,
        serverPropertiesSmoke.whitelistWorkflowOk);
    printf("world_compression_zlib=%d rle=%d lzxrle=%d input_size=%u "
        "compressed=%u rle_size=%u lzxrle_size=%u\n",
        worldCompressionSmoke.zlibRoundTripOk,
        worldCompressionSmoke.rleRoundTripOk,
        worldCompressionSmoke.lzxRleRoundTripOk,
        worldCompressionSmoke.inputSize,
        worldCompressionSmoke.compressedSize,
        worldCompressionSmoke.rleSize,
        worldCompressionSmoke.lzxRleSize);
    printf("world_system_monotonic=%d current_ms=%lld real_ms=%lld delta_ns=%lld\n",
        worldSystemSmoke.monotonicAdvanced,
        static_cast<long long>(worldSystemSmoke.currentTimeMs),
        static_cast<long long>(worldSystemSmoke.realTimeMs),
        static_cast<long long>(worldSystemSmoke.deltaNs));
    printf("delta_ms=%llu delta_ns=%llu\n",
        static_cast<unsigned long long>(deltaMs),
        static_cast<unsigned long long>(deltaNs));

    if (udpSender != LCE_INVALID_SOCKET)
    {
        LceNetCloseSocket(udpSender);
    }
    if (udpReceiver != LCE_INVALID_SOCKET)
    {
        LceNetCloseSocket(udpReceiver);
    }
    if (tcpAccepted != LCE_INVALID_SOCKET)
    {
        LceNetCloseSocket(tcpAccepted);
    }
    if (tcpClient != LCE_INVALID_SOCKET)
    {
        LceNetCloseSocket(tcpClient);
    }
    if (tcpListener != LCE_INVALID_SOCKET)
    {
        LceNetCloseSocket(tcpListener);
    }
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

    return (exists && smokeDirectoryReady &&
        resolvedExecutableDirectory &&
        !executableDirectory.empty() &&
        DirectoryExists(executableDirectory.c_str()) &&
        netInitialized && ipv4Literal &&
        ipv6Literal && !invalidLiteral &&
        worldSaveMatchById.matchedIndex == 1 &&
        worldSaveMatchById.matchedBySaveId &&
        worldSaveMatchByName.matchedIndex == 1 &&
        !worldSaveMatchByName.matchedBySaveId &&
        worldSaveNoMatch.matchedIndex == -1 &&
        !worldSaveNoMatch.matchedBySaveId &&
        defaultWorldTarget.worldName == L"world" &&
        defaultWorldTarget.saveId.empty() &&
        configuredWorldTarget.worldName == L"Smoke World" &&
        configuredWorldTarget.saveId == "SMOKE_WORLD" &&
        configuredWorldStoragePlan.worldName == L"Smoke World" &&
        configuredWorldStoragePlan.saveId == "SMOKE_WORLD" &&
        !configuredWorldStoragePlan.shouldResetSaveData &&
        loadedWorldStoragePlan.saveId == "resolved_world" &&
        !loadedWorldStoragePlan.shouldResetSaveData &&
        fallbackLoadedWorldStoragePlan.saveId == "SMOKE_WORLD" &&
        !fallbackLoadedWorldStoragePlan.shouldResetSaveData &&
        createdWorldStoragePlan.saveId == "SMOKE_WORLD" &&
        createdWorldStoragePlan.shouldResetSaveData &&
        appliedConfiguredWorldStorage &&
        appliedLoadedWorldStorage &&
        appliedCreatedWorldStorage &&
        worldStorageHookContext.appliedWorldName == L"Smoke World" &&
        worldStorageHookContext.appliedSaveId == "SMOKE_WORLD" &&
        worldStorageHookContext.resetCount == 1 &&
        worldLoadPipelineStatus ==
            ServerRuntime::eDedicatedServerWorldLoad_Loaded &&
        worldLoadPipelineContext.beginQueryCount == 1 &&
        worldLoadPipelineContext.beginLoadCount == 1 &&
        worldLoadPipelineContext.matchedIndex == 1 &&
        worldLoadPayload.matchedTitle == L"Smoke World" &&
        worldLoadPayload.matchedFilename == L"resolved_world" &&
        worldLoadPayload.resolvedSaveId == "resolved_world" &&
        worldLoadPayload.bytes.size() == 4 &&
        ownedLoadSaveDataCopied &&
        worldLoadPipelineNotFoundStatus ==
            ServerRuntime::eDedicatedServerWorldLoad_NotFound &&
        worldLoadPipelineNotFoundContext.beginQueryCount == 1 &&
        worldLoadPipelineNotFoundContext.beginLoadCount == 0 &&
        worldLoadPipelineCorruptStatus ==
            ServerRuntime::eDedicatedServerWorldLoad_Failed &&
        worldLoadPipelineCorruptContext.beginQueryCount == 1 &&
        worldLoadPipelineCorruptContext.beginLoadCount == 1 &&
        loadedWorldTargetBootstrap.status ==
            ServerRuntime::eWorldBootstrap_Loaded &&
        loadedWorldTargetBootstrap.saveData == fakeBootstrapSaveData &&
        loadedWorldTargetBootstrap.resolvedSaveId == "resolved_world" &&
        createdWorldTargetBootstrap.status ==
            ServerRuntime::eWorldBootstrap_CreatedNew &&
        createdWorldTargetBootstrap.saveData == NULL &&
        createdWorldTargetBootstrap.resolvedSaveId == "SMOKE_WORLD" &&
        failedWorldTargetBootstrap.status ==
            ServerRuntime::eWorldBootstrap_Failed &&
        failedWorldTargetBootstrap.saveData == NULL &&
        failedWorldTargetBootstrap.resolvedSaveId.empty() &&
        runHooks.afterStartupProc == &IncrementHookCount &&
        runHooks.afterStartupContext == &runAfterStartupCount &&
        runHooks.beforeSessionProc == &IncrementHookCount &&
        runHooks.beforeSessionContext == &runBeforeSessionCount &&
        runHooks.pollProc == &GameplayLoopRunPoll &&
        runHooks.pollContext == &runExecutionPollContext &&
        runHooks.afterSessionProc == &IncrementHookCount &&
        runHooks.afterSessionContext == &runAfterSessionCount &&
        worldActionWaitIdleOk &&
        waitHookIdleContext.tickCount == 2 &&
        waitHookIdleContext.handleCount == 2 &&
        worldActionWaitTimedOut &&
        waitHookTimeoutContext.tickCount == 1 &&
        waitHookTimeoutContext.handleCount == 1 &&
        worldActionWaitHalted &&
        waitHookHaltContext.tickCount == 0 &&
        waitHookHaltContext.handleCount == 0 &&
        udpReceiver != LCE_INVALID_SOCKET &&
        udpReceiverReuse && udpReceiverTimeout && udpReceiverBound &&
        udpReceiverPort > 0 && udpSender != LCE_INVALID_SOCKET &&
        udpSenderBroadcast &&
        udpSent == static_cast<int>(sizeof(udpPayload) - 1) &&
        udpPayloadOk && udpSenderIp[0] != '\0' && udpSenderPort > 0 &&
        tcpListener != LCE_INVALID_SOCKET && tcpListenerReuse &&
        tcpListenerBound && tcpListenerPort > 0 &&
        tcpResolvedLocalhost && resolvedLocalhost[0] != '\0' &&
        tcpListening &&
        tcpClient != LCE_INVALID_SOCKET && tcpClientNoDelay &&
        tcpClientTimeout && tcpConnected &&
        tcpAccepted != LCE_INVALID_SOCKET && tcpAcceptedNoDelay &&
        tcpAcceptedTimeout && tcpAcceptedIp[0] != '\0' &&
        tcpAcceptedPort > 0 && tcpClientSent && tcpServerReceived &&
        tcpServerPayloadOk && tcpServerSent && tcpClientReceived &&
        tcpClientPayloadOk &&
        lanBroadcastBuilt && lanBroadcastDecoded && lanAdded &&
        !lanUpdatedAdded && lanSessions.empty() &&
        expiredLanSessions.size() == 1 &&
        lanBroadcast.maxPlayers == 255 &&
        lanWireSizeOk &&
        lanUpdated.playerCount == 4 &&
        lanSession.isJoinable &&
        lanSession.hostPort == 25565 &&
        lanSession.netVersion == 45 &&
        expiredLanSessions[0].playerCount == 4 &&
        recursiveEnter1 == TRUE &&
        recursiveEnter2 == TRUE && tlsSet == TRUE && tlsResolved == 1 &&
        !smokeUtf8.empty() && smokeRoundTrip == smokeWide &&
        parsedUnsignedOk && parsedUnsigned == 12345ULL &&
        parsedUtcOk && parsedUtcFileTime > 0 && parsedUtcFileTime <= utcFileTime &&
        defaultDedicatedConfig.port == 25565 &&
        std::strcmp(defaultDedicatedConfig.bindIP, "0.0.0.0") == 0 &&
        std::strcmp(defaultDedicatedConfig.name, "DedicatedServer") == 0 &&
        defaultDedicatedConfig.maxPlayers == 256 &&
        !defaultDedicatedConfig.hasSeed &&
        defaultDedicatedConfig.logLevel == ServerRuntime::eServerLogLevel_Info &&
        propertyDedicatedConfig.port == dedicatedProperties.serverPort &&
        std::strcmp(
            propertyDedicatedConfig.bindIP,
            dedicatedProperties.serverIp.c_str()) == 0 &&
        std::strcmp(
            propertyDedicatedConfig.name,
            dedicatedProperties.serverName.c_str()) == 0 &&
        propertyDedicatedConfig.maxPlayers ==
            dedicatedProperties.maxPlayers &&
        propertyDedicatedConfig.worldSize ==
            dedicatedProperties.worldSize &&
        propertyDedicatedConfig.worldSizeChunks ==
            dedicatedProperties.worldSizeChunks &&
        propertyDedicatedConfig.worldHellScale ==
            dedicatedProperties.worldHellScale &&
        propertyDedicatedConfig.hasSeed &&
        propertyDedicatedConfig.seed == dedicatedProperties.seed &&
        propertyDedicatedConfig.logLevel ==
            dedicatedProperties.logLevel &&
        dedicatedParseOk && dedicatedParseError.empty() &&
        cliDedicatedConfig.port == 25571 &&
        std::strcmp(cliDedicatedConfig.bindIP, "127.0.0.1") == 0 &&
        std::strcmp(cliDedicatedConfig.name, "NativeCli") == 0 &&
        cliDedicatedConfig.maxPlayers == 32 &&
        cliDedicatedConfig.hasSeed &&
        cliDedicatedConfig.seed == 123456789LL &&
        cliDedicatedConfig.logLevel == ServerRuntime::eServerLogLevel_Debug &&
        dedicatedHelpOk && dedicatedHelpError.empty() &&
        helpDedicatedConfig.showHelp &&
        dedicatedEphemeralOk && dedicatedEphemeralError.empty() &&
        ephemeralDedicatedConfig.port == 0 &&
        !dedicatedInvalidOk && !dedicatedInvalidError.empty() &&
        !dedicatedUsageLines.empty() &&
        dedicatedUsageLines[0].find("Minecraft.Server") != std::string::npos &&
        nativeWorldLoadStorageRuntimeReady &&
        nativeWorldLoadStorageRuntimeDestroyed &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_DIFFICULTY) ==
            2U &&
        ((sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_GAMETYPE) >>
            4U) == 1U &&
        (sessionConfig.hostSettings &
            GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS) != 0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_GAMERTAGS) !=
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_LEVELTYPE) !=
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_STRUCTURES) ==
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_BONUSCHEST) !=
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_PVP) != 0U &&
        (sessionConfig.hostSettings &
            GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS) == 0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_TNT) != 0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_FIRESPREADS) ==
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_HOSTFLY) !=
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_HOSTHUNGER) ==
            0U &&
        (sessionConfig.hostSettings &
            GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE) != 0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_BEDROCKFOG) !=
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_DISABLESAVE) !=
            0U &&
        ((sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_WORLDSIZE) >>
            GAME_HOST_OPTION_BITMASK_WORLDSIZE_BITSHIFT) == 4U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_MOBGRIEFING) !=
            0U &&
        (sessionConfig.hostSettings &
            GAME_HOST_OPTION_BITMASK_KEEPINVENTORY) != 0U &&
        (sessionConfig.hostSettings &
            GAME_HOST_OPTION_BITMASK_DOMOBSPAWNING) == 0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_DOMOBLOOT) !=
            0U &&
        (sessionConfig.hostSettings & GAME_HOST_OPTION_BITMASK_DOTILEDROPS) ==
            0U &&
        (sessionConfig.hostSettings &
            GAME_HOST_OPTION_BITMASK_NATURALREGEN) != 0U &&
        (sessionConfig.hostSettings &
            GAME_HOST_OPTION_BITMASK_DODAYLIGHTCYCLE) == 0U &&
        sessionConfig.maxPlayers == 256 &&
        sessionConfig.networkMaxPlayers == 255U &&
        sessionConfig.hasSeed &&
        sessionConfig.seed == 4444 &&
        sessionConfig.worldSizeChunks == 320U &&
        sessionConfig.worldHellScale == 8U &&
        appSessionRuntimeResult.applied &&
        appSessionRuntimeResult.initializedGameSettings &&
        appSessionRuntimeResult.appliedWorldSize &&
        appSessionRuntimeResult.saveDisabled == appSessionPlan.saveDisabled &&
        loadedWorldPlan.targetWorldName == L"world" &&
        loadedWorldPlan.shouldPersistResolvedSaveId &&
        loadedWorldPlan.resolvedSaveId == "world_loaded" &&
        !loadedWorldPlan.createdNewWorld &&
        !loadedWorldPlan.loadFailed &&
        worldLoadExecutionDirectoryReady &&
        loadedWorldExecution.updatedSaveId &&
        loadedWorldExecution.savedResolvedSaveId &&
        !loadedWorldExecution.abortedStartup &&
        failedWorldExecution.abortedStartup &&
        failedWorldExecution.abortExitCode == 4 &&
        worldLoadExecutionText.find("level-id=world_loaded") !=
            std::string::npos &&
        nativeWorldManagerDirectoryReady &&
        nativeWorldManagerBootstrap.status ==
            ServerRuntime::eWorldBootstrap_CreatedNew &&
        nativeWorldManagerBootstrap.saveData == nullptr &&
        nativeWorldManagerBootstrap.resolvedSaveId ==
            "NATIVE_SMOKE_WORLD" &&
        nativeWorldManagerStorageRootReady &&
        nativeLoadedWorldManagerDirectoryReady &&
        nativeLoadedWorldManagerBootstrap.status ==
            ServerRuntime::eWorldBootstrap_Loaded &&
        nativeLoadedWorldManagerBootstrap.saveData == nullptr &&
        nativeLoadedWorldManagerPayloadCopied &&
        nativeLoadedWorldManagerBootstrap.resolvedSaveId ==
            "NATIVE_LOADED_WORLD" &&
        nativeLoadedWorldManagerStorageRootReady &&
        nativeCorruptWorldManagerDirectoryReady &&
        nativeCorruptWorldManagerBootstrap.status ==
            ServerRuntime::eWorldBootstrap_Failed &&
        nativeCorruptWorldManagerBootstrap.saveData == nullptr &&
        nativeCorruptWorldManagerBootstrap.resolvedSaveId.empty() &&
        nativeCorruptWorldManagerStorageRootReady &&
        startupExecutionDirectoryReady &&
        !startupExecutionResult.abortedStartup &&
        startupExecutionResult.appSession.applied &&
        startupExecutionResult.worldLoadExecution.savedResolvedSaveId &&
        startupExecutionResult.hostedGameStartup.startupResult == 0 &&
        startupExecutionText.find("level-id=world_loaded") !=
            std::string::npos &&
        startupExecutionFailed.abortedStartup &&
        startupExecutionFailed.abortExitCode == 4 &&
        !createdWorldPlan.shouldPersistResolvedSaveId &&
        createdWorldPlan.createdNewWorld &&
        !createdWorldPlan.loadFailed &&
        !failedWorldPlan.shouldPersistResolvedSaveId &&
        !failedWorldPlan.createdNewWorld &&
        failedWorldPlan.loadFailed &&
        networkInitPlan.seed == 7777 &&
        networkInitPlan.saveData == fakeSaveData &&
        networkInitPlan.settings == sessionConfig.hostSettings &&
        networkInitPlan.dedicatedNoLocalHostPlayer &&
        networkInitPlan.worldSizeChunks == sessionConfig.worldSizeChunks &&
        networkInitPlan.worldHellScale == sessionConfig.worldHellScale &&
        networkInitPlan.networkMaxPlayers ==
            sessionConfig.networkMaxPlayers &&
        shouldRunInitialSave &&
        shouldSkipInitialSaveWhenStopping &&
        autosaveState.intervalMs == 45000U &&
        autosaveState.nextTickMs == 91000U &&
        !autosaveState.autosaveRequested &&
        autosaveTriggerBefore &&
        autosaveTriggerAtDeadline &&
        autosaveRequested &&
        autosaveNextAdvanced &&
        autosaveCompleted &&
        shutdownPlanWithServer.shouldSetSaveOnExit &&
        shutdownPlanWithServer.shouldLogShutdownSave &&
        shutdownPlanWithServer.shouldWaitForServerStop &&
        !shutdownPlanWithoutServer.shouldSetSaveOnExit &&
        !shutdownPlanWithoutServer.shouldLogShutdownSave &&
        !shutdownPlanWithoutServer.shouldWaitForServerStop &&
        wroteSmokeFile && readSmokeFile && smokeFileReadback == smokeFileText &&
        runtimeState.hostNameUtf8 == "RuntimeHost" &&
        runtimeState.hostNameWide == L"RuntimeHost" &&
        runtimeState.bindIp == "192.168.0.8" &&
        runtimeState.multiplayerPort == 25590 &&
        runtimeState.dedicatedServerPort == 25590 &&
        runtimeState.multiplayerHost &&
        !runtimeState.multiplayerJoin &&
        runtimeState.dedicatedServer &&
        runtimeState.lanAdvertise &&
        autosaveTrackerPendingSnapshot.autosavePending &&
        !autosaveTrackerPendingSnapshot.lastObservedIdle &&
        autosaveTrackerPendingSnapshot.requestCount == 2 &&
        autosaveTrackerPendingSnapshot.completionCount == 0 &&
        !autosaveTrackerCompletedSnapshot.autosavePending &&
        autosaveTrackerCompletedSnapshot.lastObservedIdle &&
        autosaveTrackerCompletedSnapshot.requestCount == 2 &&
        autosaveTrackerCompletedSnapshot.completionCount == 1 &&
        platformActionIdle &&
        platformActionPending &&
        platformWaitStillBusy &&
        platformWaitIdle &&
        platformAutosaveRequestsAfterWait == 1 &&
        platformAutosaveCompletionsAfterWait == 1 &&
        platformGameplayInstance &&
        platformAppShutdownBefore &&
        platformAppShutdownAfter &&
        platformGameplayHaltedBefore &&
        platformGameplayHaltedAfter &&
        platformFlagsProjected &&
        platformStopSignalValid &&
        platformHostedRuntimeResult.ok &&
        restartedHostedRuntimeResult.ok &&
        initialSaveExecution.requested &&
        initialSaveExecution.completed &&
        !initialSaveExecution.timedOut &&
        shutdownExecution.plan.shouldSetSaveOnExit &&
        shutdownExecution.plan.shouldLogShutdownSave &&
        shutdownExecution.plan.shouldWaitForServerStop &&
        shutdownExecution.haltedGameplay &&
        gameplayLoopRequest.autosaveRequested &&
        !gameplayLoopRequest.shouldExit &&
        !gameplayLoopPending.autosaveCompleted &&
        !gameplayLoopPending.autosaveRequested &&
        !gameplayLoopPending.shouldExit &&
        gameplayLoopComplete.autosaveCompleted &&
        !gameplayLoopComplete.shouldExit &&
        gameplayLoopExit.shouldExit &&
        gameplayLoopRunResult.iterations == 3U &&
        gameplayLoopRunPollContext.pollCount == 3 &&
        gameplayLoopRunResult.requestedAppShutdown &&
        gameplayLoopRunResult.lastIteration.shouldExit &&
        nativeHostedCoreStartupResult.exitCode == 0 &&
        !nativeHostedCoreStartupResult.sessionSnapshot.loadedFromSave &&
        nativeHostedCoreStartupResult.sessionSnapshot.payloadValidated &&
        nativeHostedCoreStartupResult.sessionSnapshot
                .startupThreadIterations == 2U &&
        nativeHostedCoreStartupResult.sessionSnapshot
                .startupThreadDurationMs > 0U &&
        nativeHostedCoreStartupResult.sessionSnapshot.active &&
        !nativeHostedCoreStartupResult.sessionSnapshot.hostedThreadActive &&
        nativeHostedCoreStartupResult.sessionSnapshot.runtimePhase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Startup &&
        nativeHostedCoreFrameStarted &&
        nativeHostedCoreFrameFirst.frameTimestampMs > 0U &&
        nativeHostedCoreFrameFirst.workerFrame.autosaveCompletions == 0U &&
        nativeHostedCoreFrameFirst.workerFrame.nextSleepDurationMs == 10U &&
        !nativeHostedCoreFrameFirst.workerFrame.shutdownRequested &&
        !nativeHostedCoreFrameFirst.workerFrame.shouldStopRunning &&
        nativeHostedCoreFrameFirst.workerFrame.snapshot
                .pendingWorldActionTicks == 1U &&
        nativeHostedCoreFrameFirst.workerFrame.snapshot
                .processedAutosaveCommands == 0U &&
        !nativeHostedCoreFrameFirst.workerFrame.idle &&
        nativeHostedCoreFrameFirst.sessionSnapshot.sessionTicks == 1U &&
        nativeHostedCoreFrameFirst.sessionSnapshot.gameplayLoopIterations ==
            1U &&
        nativeHostedCoreFrameFirst.sessionSnapshot.hostedThreadActive &&
        nativeHostedCoreFrameFirst.sessionSnapshot.runtimePhase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Running &&
        nativeHostedCoreFrameSecond.frameTimestampMs >=
            nativeHostedCoreFrameFirst.frameTimestampMs &&
        nativeHostedCoreFrameSecond.workerFrame.autosaveCompletions == 1U &&
        nativeHostedCoreFrameSecond.workerFrame.nextSleepDurationMs == 10U &&
        !nativeHostedCoreFrameSecond.workerFrame.shutdownRequested &&
        !nativeHostedCoreFrameSecond.workerFrame.shouldStopRunning &&
        nativeHostedCoreFrameSecond.workerFrame.snapshot
                .pendingWorldActionTicks == 0U &&
        nativeHostedCoreFrameSecond.workerFrame.snapshot
                .processedAutosaveCommands == 1U &&
        nativeHostedCoreFrameSecond.workerFrame.idle &&
        nativeHostedCoreFrameSecond.sessionSnapshot.sessionTicks == 2U &&
        nativeHostedCoreFrameSecond.sessionSnapshot.gameplayLoopIterations ==
            2U &&
        nativeHostedCoreFrameSecond.sessionSnapshot
                .observedAutosaveCompletions == 1U &&
        nativeHostedCoreFrameSecond.sessionSnapshot.hostedThreadActive &&
        nativeHostedCoreFrameSecond.sessionSnapshot.runtimePhase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Running &&
        !nativeHostedCoreFrameStoppedSnapshot.active &&
        nativeHostedCoreFrameStoppedSnapshot.runtimePhase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Stopped &&
        nativeHostedCoreRunResult.exitCode == 0 &&
        nativeHostedCoreRunResult.startup.sessionSnapshot
                .payloadValidated &&
        nativeHostedCoreRunResult.startup.sessionSnapshot
                .startupThreadIterations == 2U &&
        nativeHostedCoreRunResult.startup.sessionSnapshot
                .startupThreadDurationMs > 0U &&
        nativeHostedCoreRunResult.loopIterations == 1U &&
        nativeHostedCoreRunResult.finalState.autosaveCompletions == 0U &&
        nativeHostedCoreRunResult.lastFrame.frameTimestampMs > 0U &&
        nativeHostedCoreRunResult.lastFrame.workerFrame.nextSleepDurationMs ==
            0U &&
        nativeHostedCoreRunResult.lastFrame.workerFrame.shouldStopRunning &&
        nativeHostedCoreRunResult.finalState.workerSnapshot
            .pendingWorldActionTicks ==
            0U &&
        nativeHostedCoreRunResult.finalState.workerSnapshot
            .pendingAutosaveCommands ==
            0U &&
        nativeHostedCoreRunResult.finalState.workerSnapshot
            .pendingSaveCommands ==
            0U &&
        nativeHostedCoreRunResult.finalState.workerSnapshot
            .pendingStopCommands ==
            0U &&
        nativeHostedCoreRunResult.finalState.workerSnapshot
            .pendingHaltCommands ==
            0U &&
        nativeHostedCoreRunResult.finalState.workerSnapshot
            .activeCommandKind ==
            ServerRuntime::eNativeDedicatedServerHostedGameWorkerCommand_None &&
        g_nativeHostedCoreHookSmokeContext.readyCount == 1 &&
        g_nativeHostedCoreHookSmokeContext.stoppedCount == 1 &&
        !nativeHostedCoreSnapshot.active &&
        nativeHostedCoreSnapshot.runtimePhase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Stopped &&
        hostedGameRuntimeNullThreadResult == -1 &&
        hostedGameRuntimeNullThreadSnapshot.startAttempted &&
        !hostedGameRuntimeNullThreadSnapshot.threadInvoked &&
        hostedGameRuntimeNullThreadSnapshot.startupResult == -1 &&
        hostedGameRuntimeNullThreadSnapshot.phase ==
            ServerRuntime::eDedicatedServerHostedGameRuntimePhase_Failed &&
        hostedGameRuntimeSleepResult == 13 &&
        hostedGameRuntimeSleepThreadValue == 2 &&
        nativeHostedStubResult == 0 &&
        nativeHostedSaveTextBuilt &&
        nativeHostedStubInitData.seed == hostedGamePlan.resolvedSeed &&
        nativeHostedStubInitData.saveData == nativeHostedSaveData &&
        nativeHostedStubInitData.settings == sessionConfig.hostSettings &&
        nativeHostedStubInitData.dedicatedNoLocalHostPlayer &&
        nativeHostedStubInitData.xzSize == sessionConfig.worldSizeChunks &&
        nativeHostedStubInitData.hellScale == sessionConfig.worldHellScale &&
        nativeHostedStubTickAfter > nativeHostedStubTickBefore &&
        nativeHostedStubSnapshot.startAttempted &&
        nativeHostedStubSnapshot.threadInvoked &&
        nativeHostedStubSnapshot.startupResult == 0 &&
        nativeHostedStubSnapshot.startupPayloadPresent &&
        nativeHostedStubSnapshot.startupPayloadValidated &&
        nativeHostedStubSnapshot.startupThreadIterations == 4U &&
        nativeHostedStubSnapshot.startupThreadDurationMs > 0U &&
        nativeHostedStubSnapshot.hostedThreadActive &&
        nativeHostedStubSnapshot.loadedSaveMetadataAvailable &&
        nativeHostedStubSnapshot.previousStartupMode == "loaded" &&
        !nativeHostedStubSnapshot.previousStartupPayloadPresent &&
        !nativeHostedStubSnapshot.previousStartupPayloadValidated &&
        nativeHostedStubSnapshot.previousStartupThreadIterations == 0U &&
        nativeHostedStubSnapshot.previousStartupThreadDurationMs == 0U &&
        nativeHostedStubStopped &&
        nativeHostedStubExitCode == 0 &&
        nativeHostedStubThreadTicks > 0U &&
        !nativeHostedStubStoppedSnapshot.hostedThreadActive &&
        nativeHostedStubStoppedSnapshot.hostedThreadTicks >=
            nativeHostedStubThreadTicks &&
        hostedGameRuntimeResult == 11 &&
        hostedGameRuntimeThreadValue == 1 &&
        hostedGameStartupExecution.startupResult == 0 &&
        !hostedGameStartupExecution.startupPlan.shouldAbortStartup &&
        hostedGameStartupExecution.startupPlan.abortExitCode == 0 &&
        hostedGameStartupInitData.seed == hostedGamePlan.resolvedSeed &&
        hostedGameStartupInitData.saveData == fakeSaveData &&
        hostedGameStartupInitData.settings == sessionConfig.hostSettings &&
        hostedGameStartupInitData.dedicatedNoLocalHostPlayer &&
        hostedGameStartupInitData.xzSize == sessionConfig.worldSizeChunks &&
        hostedGameStartupInitData.hellScale ==
            sessionConfig.worldHellScale &&
        hostedGameSessionOk &&
        platformSessionRuntimeResult.ok &&
        sessionExecution.initialSave.requested &&
        sessionExecution.initialSave.completed &&
        !sessionExecution.initialSave.timedOut &&
        sessionExecution.autosaveState.intervalMs == 45000U &&
        sessionExecution.gameplayLoop.iterations == 3U &&
        sessionExecution.gameplayLoop.requestedAppShutdown &&
        sessionExecution.gameplayLoop.lastIteration.shouldExit &&
        sessionExecution.shutdown.haltedGameplay &&
        sessionExecution.shutdown.plan.shouldSetSaveOnExit &&
        sessionExecution.shutdown.plan.shouldWaitForServerStop &&
        sessionGameplayHalted &&
        sessionPollContext.pollCount == 3 &&
        platformRunRuntimeResult.ok &&
        !runExecution.abortedStartup &&
        runExecution.completedSession &&
        runExecution.startup.appSession.applied &&
        runExecution.session.initialSave.requested &&
        runExecution.session.initialSave.completed &&
        runExecution.session.gameplayLoop.iterations == 3U &&
        runExecution.session.shutdown.haltedGameplay &&
        runExecutionGameplayHalted &&
        runAfterStartupCount == 1 &&
        runBeforeSessionCount == 1 &&
        runAfterSessionCount == 1 &&
        runExecutionPollContext.pollCount == 3 &&
        defaultAutosaveMs == 60000U &&
        configuredAutosaveMs == 45000U &&
        nextAutosaveTick == 46000U &&
        std::strcmp(storagePlatformDirectory, "NativeDesktop") == 0 &&
        storageGameHddRoot == "NativeDesktop/GameHDD" &&
        overriddenStorageRoot == "/tmp/minecraft-native-storage-smoke" &&
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
        bootstrapPrepared && bootstrapEnvironmentReady &&
        bootstrapAccessReady && bootstrapAccessShutdown &&
        bootstrapRestoredDirectory &&
        bootstrapSmokeContext.runtimeState.hostNameUtf8 == "BootstrapSmoke" &&
        bootstrapSmokeContext.runtimeState.bindIp == "127.0.0.1" &&
        bootstrapSmokeContext.runtimeState.multiplayerPort == 25578 &&
        bootstrapSmokeContext.worldTarget.worldName == L"world" &&
        bootstrapSmokeContext.worldTarget.saveId == "world" &&
        bootstrapSmokeContext.storageRoot == "NativeDesktop/GameHDD" &&
        socketBootstrapStarted && socketBootstrapConnected &&
        socketBootstrapAcceptedOk && socketBootstrapStopped &&
        socketBootstrapBoundPort > 0 &&
        utcTimestamp.size() == 20 && utcTimestamp[10] == 'T' &&
        utcTimestamp[19] == 'Z' &&
        worldFileHeaderSmoke.duplicateReused &&
        worldFileHeaderSmoke.roundTripOk &&
        worldFileHeaderSmoke.prefixesOk &&
        worldFileHeaderSmoke.offsetsOk &&
        worldFileHeaderSmoke.fileCount == 2 &&
        worldFileHeaderSmoke.totalSize > 12U &&
        serverPropertiesSmoke.createdDefaults &&
        serverPropertiesSmoke.normalizedValues &&
        serverPropertiesSmoke.savePersisted &&
        serverPropertiesSmoke.whitelistWorkflowOk &&
        worldCompressionSmoke.zlibRoundTripOk &&
        worldCompressionSmoke.rleRoundTripOk &&
        worldCompressionSmoke.lzxRleRoundTripOk &&
        worldCompressionSmoke.inputSize > 0U &&
        worldCompressionSmoke.compressedSize > 0U &&
        worldCompressionSmoke.rleSize > 0U &&
        worldCompressionSmoke.lzxRleSize > 0U &&
        worldSystemSmoke.monotonicAdvanced &&
        worldSystemSmoke.currentTimePositive &&
        worldSystemSmoke.realTimePositive &&
        worldSystemSmoke.deltaNs > 0 &&
        waitAny == WAIT_OBJECT_0 + 1 && waitAllBefore == WAIT_TIMEOUT &&
        waitAllAfter == WAIT_OBJECT_0 && resumed == 0 &&
        threadWait == WAIT_OBJECT_0 && gotExitCode == TRUE &&
        threadExitCode == 7 && threadValue == 1 && deltaMs > 0 && deltaNs > 0)
        ? (g_expectedHostedGameSaveData = nullptr,
            ServerRuntime::DestroyLoadSaveDataThreadParam(
                nativeHostedSaveData),
            ServerRuntime::DestroyLoadSaveDataThreadParam(fakeSaveData),
            0)
        : (g_expectedHostedGameSaveData = nullptr,
            ServerRuntime::DestroyLoadSaveDataThreadParam(
                nativeHostedSaveData),
            ServerRuntime::DestroyLoadSaveDataThreadParam(fakeSaveData),
            1);
}
