#include <cstdio>
#include <cstring>
#include <filesystem>

#include "Minecraft.Client/Common/App_Defines.h"
#include "Minecraft.Server/Access/Access.h"
#include "Minecraft.Server/Access/BanManager.h"
#include "Minecraft.Server/Common/DedicatedServerBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformState.h"
#include "Minecraft.Server/Common/DedicatedServerSessionConfig.h"
#include "Minecraft.Server/Common/DedicatedServerSocketBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerWorldBootstrap.h"
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
#include "Minecraft.Server/vendor/linenoise/linenoise.h"
#include "Minecraft.World/PerformanceTimer.h"
#include "ServerPropertiesSmoke.h"
#include "WorldCompressionSmoke.h"
#include "Minecraft.World/ThreadName.h"
#include "WorldFileHeaderSmoke.h"
#include "WorldSystemSmoke.h"

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
        SetThreadName(static_cast<DWORD>(-1), "smoke-worker");
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
    ServerRuntime::WorldBootstrapResult createdWorldBootstrap;
    createdWorldBootstrap.status = ServerRuntime::eWorldBootstrap_CreatedNew;
    createdWorldBootstrap.resolvedSaveId = "world";
    const ServerRuntime::DedicatedServerWorldBootstrapPlan createdWorldPlan =
        ServerRuntime::BuildDedicatedServerWorldBootstrapPlan(
            worldBootstrapProperties,
            createdWorldBootstrap);
    ServerRuntime::WorldBootstrapResult failedWorldBootstrap;
    failedWorldBootstrap.status = ServerRuntime::eWorldBootstrap_Failed;
    const ServerRuntime::DedicatedServerWorldBootstrapPlan failedWorldPlan =
        ServerRuntime::BuildDedicatedServerWorldBootstrapPlan(
            worldBootstrapProperties,
            failedWorldBootstrap);
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
    LoadSaveDataThreadParam *fakeSaveData =
        reinterpret_cast<LoadSaveDataThreadParam *>(0x1234);
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
    const ServerRuntime::DedicatedServerShutdownPlan shutdownPlanWithServer =
        ServerRuntime::BuildDedicatedServerShutdownPlan(
            true,
            true);
    const ServerRuntime::DedicatedServerShutdownPlan shutdownPlanWithoutServer =
        ServerRuntime::BuildDedicatedServerShutdownPlan(
            false,
            false);
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
    printf("dedicated_defaults=%d dedicated_props=%d dedicated_cli=%d "
        "dedicated_help=%d dedicated_invalid=%d usage_lines=%zu\n",
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
        !dedicatedInvalidOk &&
            !dedicatedInvalidError.empty(),
        dedicatedUsageLines.size());
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
    printf("world_bootstrap_plan=%d loaded_persist=%d created_new=%d "
        "failed=%d init_plan=%d init_seed=%lld init_players=%u "
        "hosted_plan=%d hosted_seed=%lld\n",
        loadedWorldPlan.targetWorldName == L"world" &&
            loadedWorldPlan.shouldPersistResolvedSaveId &&
            loadedWorldPlan.resolvedSaveId == "world_loaded" &&
            !loadedWorldPlan.createdNewWorld &&
            !loadedWorldPlan.loadFailed,
        loadedWorldPlan.shouldPersistResolvedSaveId,
        createdWorldPlan.createdNewWorld,
        failedWorldPlan.loadFailed,
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
            hostedGamePlan.fakeLocalPlayerJoined,
        static_cast<long long>(hostedGamePlan.resolvedSeed));
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
        shouldRunInitialSave,
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
    printf("server_storage_platform=%s game_hdd_root=%s\n",
        storagePlatformDirectory,
        storageGameHddRoot.c_str());
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
        "port=%d storage_root=%s\n",
        bootstrapPrepared,
        bootstrapEnvironmentReady,
        bootstrapAccessReady,
        bootstrapAccessShutdown,
        bootstrapRestoredDirectory,
        bootstrapSmokeContext.runtimeState.hostNameUtf8.c_str(),
        bootstrapSmokeContext.runtimeState.bindIp.c_str(),
        bootstrapSmokeContext.runtimeState.multiplayerPort,
        bootstrapSmokeContext.storageRoot.c_str());
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
        !dedicatedInvalidOk && !dedicatedInvalidError.empty() &&
        !dedicatedUsageLines.empty() &&
        dedicatedUsageLines[0].find("Minecraft.Server") != std::string::npos &&
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
        loadedWorldPlan.targetWorldName == L"world" &&
        loadedWorldPlan.shouldPersistResolvedSaveId &&
        loadedWorldPlan.resolvedSaveId == "world_loaded" &&
        !loadedWorldPlan.createdNewWorld &&
        !loadedWorldPlan.loadFailed &&
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
        defaultAutosaveMs == 60000U &&
        configuredAutosaveMs == 45000U &&
        nextAutosaveTick == 46000U &&
        std::strcmp(storagePlatformDirectory, "NativeDesktop") == 0 &&
        storageGameHddRoot == "NativeDesktop/GameHDD" &&
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
        ? 0
        : 1;
}
