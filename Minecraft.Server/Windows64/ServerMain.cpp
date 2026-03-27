#include "stdafx.h"

#include "Common/App_Defines.h"
#include "Common/Network/GameNetworkManager.h"
#include "Input.h"
#include "Minecraft.h"
#include "MinecraftServer.h"
#include "..\Common\DedicatedServerBootstrap.h"
#include "..\Common\DedicatedServerOptions.h"
#include "..\Common\DedicatedServerPlatformState.h"
#include "..\Common\DedicatedServerRuntime.h"
#include "..\Common\DedicatedServerShutdownPlan.h"
#include "..\Common\DedicatedServerSessionConfig.h"
#include "..\Common\DedicatedServerWorldBootstrap.h"
#include "..\Common\StringUtils.h"
#include "..\ServerLogger.h"
#include "..\ServerLogManager.h"
#include "..\ServerProperties.h"
#include "..\ServerShutdown.h"
#include "..\WorldManager.h"
#include "..\Console\ServerCli.h"
#include <lce_process/lce_process.h>
#include <lce_time/lce_time.h>
#include "Tesselator.h"
#include "Windows64/4JLibs/inc/4J_Render.h"
#include "Windows64/GameConfig/Minecraft.spa.h"
#include "Windows64/KeyboardMouseInput.h"
#include "Windows64/Network/WinsockNetLayer.h"
#include "Windows64/Windows64_UIController.h"
#include "Common/UI/UI.h"

#include "../../Minecraft.World/AABB.h"
#include "../../Minecraft.World/Vec3.h"
#include "../../Minecraft.World/IntCache.h"
#include "../../Minecraft.World/ChunkSource.h"
#include "../../Minecraft.World/TilePos.h"
#include "../../Minecraft.World/compression.h"
#include "../../Minecraft.World/OldChunkStorage.h"
#include "../../Minecraft.World/net.minecraft.world.level.tile.h"
#include "../../Minecraft.World/Random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <cwchar>

extern ATOM MyRegisterClass(HINSTANCE hInstance);
extern BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
extern HRESULT InitDevice();
extern void CleanupDevice();
extern void DefineActions(void);

extern HWND g_hWnd;
extern int g_iScreenWidth;
extern int g_iScreenHeight;
extern char g_Win64Username[17];
extern wchar_t g_Win64UsernameW[17];
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pImmediateContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_pRenderTargetView;
extern ID3D11DepthStencilView* g_pDepthStencilView;
extern DWORD dwProfileSettingsA[];

static const int kProfileValueCount = 5;
static const int kProfileSettingCount = 4;
static std::atomic<bool> g_shutdownRequested(false);
static const int kServerActionPad = 0;

static bool IsShutdownRequested()
{
	return g_shutdownRequested.load();
}

namespace ServerRuntime
{
	void RequestDedicatedServerShutdown()
	{
		g_shutdownRequested.store(true);
	}
}

/**
 * Calls dedicated bootstrap shutdown automatically once native/bootstrap
 * environment initialization succeeded.
 * 共有ブートストラップ初期化後のShutdownを自動化する
 */
class DedicatedServerBootstrapGuard
{
public:
	DedicatedServerBootstrapGuard()
		: m_context(nullptr)
	{
	}

	void Activate(ServerRuntime::DedicatedServerBootstrapContext *context)
	{
		m_context = context;
	}

	~DedicatedServerBootstrapGuard()
	{
		ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(m_context);
	}

private:
	ServerRuntime::DedicatedServerBootstrapContext *m_context;
};
static BOOL WINAPI ConsoleCtrlHandlerProc(DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		ServerRuntime::RequestDedicatedServerShutdown();
		return TRUE;
	default:
		return FALSE;
	}
}

/**
 * **Wait For Server Stopped Signal**
 *
 * Thread entry used during shutdown to wait until the network layer reports server stop completion
 * 停止通知待機用の終了スレッド処理
 */
static int WaitForServerStoppedThreadProc(void *)
{
	if (g_NetworkManager.ServerStoppedValid())
	{
		g_NetworkManager.ServerStoppedWait();
	}
	return 0;
}

static void PrintUsage()
{
	std::vector<std::string> usageLines;
	ServerRuntime::BuildDedicatedServerUsageLines(&usageLines);
	for (size_t i = 0; i < usageLines.size(); ++i)
	{
		ServerRuntime::LogInfo("usage", usageLines[i].c_str());
	}
}

using ServerRuntime::LogError;
using ServerRuntime::LogErrorf;
using ServerRuntime::LogInfof;
using ServerRuntime::LogDebugf;
using ServerRuntime::LogStartupStep;
using ServerRuntime::LogWarn;
using ServerRuntime::LogWorldIO;
using ServerRuntime::SaveServerPropertiesConfig;
using ServerRuntime::ServerPropertiesConfig;
using ServerRuntime::TryParseServerLogLevel;
using ServerRuntime::DedicatedServerConfig;
using ServerRuntime::StringUtils::WideToUtf8;
using ServerRuntime::BootstrapWorldForServer;
using ServerRuntime::eWorldBootstrap_CreatedNew;
using ServerRuntime::eWorldBootstrap_Failed;
using ServerRuntime::eWorldBootstrap_Loaded;
using ServerRuntime::WaitForWorldActionIdle;
using ServerRuntime::WorldBootstrapResult;

/**
 * **Tick Core Async Subsystems**
 *
 * Advances core subsystems for one frame to keep async processing alive
 * Call continuously even inside wait loops to avoid stalling storage/profile/network work
 * 非同期進行維持のためのティック処理
 */
static void TickCoreSystems()
{
	g_NetworkManager.DoWork();
	ProfileManager.Tick();
	StorageManager.Tick();
}

/**
 * **Handle Queued XUI Server Action Once**
 *
 * Processes queued XUI/server action once
 * XUIアクションの単発処理
 */
static void HandleXuiActions()
{
	app.HandleXuiActions();
}

/**
 * Dedicated Server Entory Point
 *
 * 主な責務:
 * - プロセス/描画/ネットワークの初期化
 * - `WorldManager` によるワールドロードまたは新規作成
 * - メインループと定期オートセーブ実行
 * - 終了時の最終保存と各サブシステムの安全停止
 */
int main(int argc, char **argv)
{
	ServerRuntime::DedicatedServerBootstrapContext bootstrapContext = {};
	DedicatedServerBootstrapGuard bootstrapShutdownGuard;

	SetConsoleCtrlHandler(ConsoleCtrlHandlerProc, TRUE);
	std::string bootstrapError;
	switch (ServerRuntime::PrepareDedicatedServerBootstrapContext(
		argc,
		argv,
		&bootstrapContext,
		&bootstrapError))
	{
	case ServerRuntime::eDedicatedServerBootstrap_Ready:
		break;
	case ServerRuntime::eDedicatedServerBootstrap_ShowHelp:
		PrintUsage();
		return 0;
	case ServerRuntime::eDedicatedServerBootstrap_Failed:
	default:
		if (!bootstrapError.empty())
		{
			LogError("startup", bootstrapError.c_str());
		}
		PrintUsage();
		return 1;
	}

	LogStartupStep("initializing process state");
	DedicatedServerConfig &config = bootstrapContext.config;
	ServerPropertiesConfig &serverProperties =
		bootstrapContext.serverProperties;
	const ServerRuntime::DedicatedServerRuntimeState &runtimeState =
		bootstrapContext.runtimeState;
	const ServerRuntime::DedicatedServerPlatformState platformState =
		ServerRuntime::BuildDedicatedServerPlatformState(
			runtimeState,
			MINECRAFT_NET_MAX_PLAYERS);

	g_iScreenWidth = 1280;
	g_iScreenHeight = 720;

	strncpy_s(
		g_Win64Username,
		sizeof(g_Win64Username),
		platformState.userNameUtf8.c_str(),
		_TRUNCATE);
	wmemset(
		g_Win64UsernameW,
		0,
		sizeof(g_Win64UsernameW) / sizeof(g_Win64UsernameW[0]));
	wcsncpy(
		g_Win64UsernameW,
		platformState.userNameWide.c_str(),
		(sizeof(g_Win64UsernameW) / sizeof(g_Win64UsernameW[0])) - 1);
	g_Win64MultiplayerHost = platformState.multiplayerHost;
	g_Win64MultiplayerJoin = platformState.multiplayerJoin;
	g_Win64MultiplayerPort = platformState.multiplayerPort;
	strncpy_s(
		g_Win64MultiplayerIP,
		sizeof(g_Win64MultiplayerIP),
		platformState.bindIp.c_str(),
		_TRUNCATE);
	g_Win64DedicatedServer = platformState.dedicatedServer;
	g_Win64DedicatedServerPort = platformState.dedicatedServerPort;
	strncpy_s(
		g_Win64DedicatedServerBindIP,
		sizeof(g_Win64DedicatedServerBindIP),
		platformState.bindIp.c_str(),
		_TRUNCATE);
	g_Win64DedicatedServerLanAdvertise = platformState.lanAdvertise;
	LogStartupStep("initializing server log manager");
	ServerRuntime::ServerLogManager::Initialize();
	LogStartupStep("initializing dedicated access control");
	if (!ServerRuntime::InitializeDedicatedServerBootstrapEnvironment(
		&bootstrapContext,
		&bootstrapError))
	{
		LogError("startup", bootstrapError.c_str());
		return 2;
	}
	bootstrapShutdownGuard.Activate(&bootstrapContext);
	LogInfof("startup", "LAN advertise: %s", serverProperties.lanAdvertise ? "enabled" : "disabled");
	LogInfof("startup", "Whitelist: %s", serverProperties.whiteListEnabled ? "enabled" : "disabled");
	LogInfof("startup", "Spawn protection radius: %d", serverProperties.spawnProtectionRadius);
#ifdef _LARGE_WORLDS
	LogInfof(
		"startup",
		"World size preset: %d (xz=%d, hell-scale=%d)",
		config.worldSize,
		config.worldSizeChunks,
		config.worldHellScale);
#endif

	LogStartupStep("registering hidden window class");
	HINSTANCE hInstance = GetModuleHandle(NULL);
	MyRegisterClass(hInstance);

	LogStartupStep("creating hidden window");
	if (!InitInstance(hInstance, SW_HIDE))
	{
		LogError("startup", "Failed to create window instance.");
		
		return 2;
	}
	ShowWindow(g_hWnd, SW_HIDE);

	LogStartupStep("initializing graphics device wrappers");
	if (FAILED(InitDevice()))
	{
		LogError("startup", "Failed to initialize D3D device.");
		CleanupDevice();
		
		return 2;
	}

	LogStartupStep("loading media/string tables");
	app.loadMediaArchive();
	RenderManager.Initialise(g_pd3dDevice, g_pSwapChain);
	app.loadStringTable();
	ui.init(g_pd3dDevice, g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView, g_iScreenWidth, g_iScreenHeight);

	InputManager.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);
	g_KBMInput.Init();
	DefineActions();
	InputManager.SetJoypadMapVal(0, 0);
	InputManager.SetKeyRepeatRate(0.3f, 0.2f);

	ProfileManager.Initialise(
		TITLEID_MINECRAFT,
		app.m_dwOfferID,
		PROFILE_VERSION_10,
		kProfileValueCount,
		kProfileSettingCount,
		dwProfileSettingsA,
		app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
		&app.uiGameDefinedDataChangedBitmask);
	ProfileManager.SetDefaultOptionsCallback(&CConsoleMinecraftApp::DefaultOptionsCallback, (LPVOID)&app);
	ProfileManager.SetDebugFullOverride(true);

	LogStartupStep("initializing network manager");
	g_NetworkManager.Initialise();

	for (size_t i = 0; i < platformState.players.size(); ++i)
	{
		const ServerRuntime::DedicatedServerPlayerState &playerState =
			platformState.players[i];
		IQNet::m_player[i].m_smallId = static_cast<BYTE>(playerState.smallId);
		IQNet::m_player[i].m_isRemote = playerState.isRemote;
		IQNet::m_player[i].m_isHostPlayer = playerState.isHostPlayer;
		wmemset(IQNet::m_player[i].m_gamertag, 0, 32);
		wcsncpy(
			IQNet::m_player[i].m_gamertag,
			playerState.gamertag.c_str(),
			31);
	}
	WinsockNetLayer::Initialize();

	Tesselator::CreateNewThreadStorage(1024 * 1024);
	AABB::CreateNewThreadStorage();
	Vec3::CreateNewThreadStorage();
	IntCache::CreateNewThreadStorage();
	Compression::CreateNewThreadStorage();
	OldChunkStorage::CreateNewThreadStorage();
	Level::enableLightingCache();
	Tile::CreateNewThreadStorage();

	LogStartupStep("creating Minecraft singleton");
	Minecraft::main();
	Minecraft *minecraft = Minecraft::GetInstance();
	if (minecraft == NULL)
	{
		LogError("startup", "Minecraft initialization failed.");
		CleanupDevice();
		
		return 3;
	}

	const ServerRuntime::DedicatedServerSessionConfig sessionConfig =
		ServerRuntime::BuildDedicatedServerSessionConfig(
			config,
			serverProperties);
	const ServerRuntime::DedicatedServerAppSessionPlan appSessionPlan =
		ServerRuntime::BuildDedicatedServerAppSessionPlan(sessionConfig);

	MinecraftServer::resetFlags();
	if (appSessionPlan.shouldInitGameSettings)
	{
		app.InitGameSettings();
	}
	app.SetTutorialMode(appSessionPlan.tutorialMode);
	app.SetCorruptSaveDeleted(appSessionPlan.corruptSaveDeleted);
	app.SetGameHostOption(eGameHostOption_All, appSessionPlan.hostSettings);
#ifdef _LARGE_WORLDS
	// Apply desired target size for loading existing worlds.
	// Expansion happens only when target size is larger than current level size.
	if (appSessionPlan.shouldApplyWorldSize)
	{
		app.SetGameNewWorldSize(appSessionPlan.worldSizeChunks, true);
		app.SetGameNewHellScale(appSessionPlan.worldHellScale);
	}
#endif

	StorageManager.SetSaveDisabled(appSessionPlan.saveDisabled);
	// Read world name and fixed save-id from server.properties
	// Delegate load-vs-create decision to WorldManager
	WorldBootstrapResult worldBootstrap = BootstrapWorldForServer(serverProperties, kServerActionPad, &TickCoreSystems);
	const ServerRuntime::DedicatedServerWorldBootstrapPlan worldBootstrapPlan =
		ServerRuntime::BuildDedicatedServerWorldBootstrapPlan(
			serverProperties,
			worldBootstrap);
	if (worldBootstrapPlan.shouldPersistResolvedSaveId)
	{
		// Persist the actually loaded save-id back to config
		// Keep lookup keys aligned for next startup
		LogWorldIO("updating level-id to loaded save filename");
		serverProperties.worldSaveId = worldBootstrapPlan.resolvedSaveId;
		if (!SaveServerPropertiesConfig(serverProperties))
		{
			LogWorldIO("failed to persist updated level-id");
		}
	}
	else if (worldBootstrapPlan.loadFailed)
	{
		LogErrorf(
			"world-io",
			"Failed to load configured world \"%s\".",
			WideToUtf8(worldBootstrapPlan.targetWorldName).c_str());
		WinsockNetLayer::Shutdown();
		g_NetworkManager.Terminate();
		CleanupDevice();
		
		return 4;
	}

	const ServerRuntime::DedicatedServerHostedGamePlan hostedGamePlan =
		ServerRuntime::BuildDedicatedServerHostedGamePlan(
			sessionConfig,
			worldBootstrap.saveData,
			(new Random())->nextLong());
	const ServerRuntime::DedicatedServerNetworkInitPlan &networkInitPlan =
		hostedGamePlan.networkInitPlan;
	NetworkGameInitData *param = new NetworkGameInitData();
	ServerRuntime::PopulateDedicatedServerNetworkGameInitData(
		param,
		networkInitPlan);

	LogStartupStep("starting hosted network game thread");
	g_NetworkManager.HostGame(
		hostedGamePlan.localUsersMask,
		hostedGamePlan.onlineGame,
		hostedGamePlan.privateGame,
		hostedGamePlan.publicSlots,
		hostedGamePlan.privateSlots);
	if (hostedGamePlan.fakeLocalPlayerJoined)
	{
		g_NetworkManager.FakeLocalPlayerJoined();
	}

	C4JThread *startThread = new C4JThread(&CGameNetworkManager::RunNetworkGameThreadProc, (LPVOID)param, "RunNetworkGame");
	startThread->Run();

	while (startThread->isRunning() && !IsShutdownRequested())
	{
		TickCoreSystems();
		LceSleepMilliseconds(10);
	}

	startThread->WaitForCompletion(INFINITE);
	int startupResult = startThread->GetExitCode();
	delete startThread;

	if (startupResult != 0)
	{
		LogErrorf("startup", "Failed to start dedicated server (code %d).", startupResult);
		WinsockNetLayer::Shutdown();
		g_NetworkManager.Terminate();
		CleanupDevice();
		
		return 4;
	}

	LogStartupStep("server startup complete");
	LogInfof("startup", "Dedicated server listening on %s:%d", g_Win64MultiplayerIP, g_Win64MultiplayerPort);
	if (ServerRuntime::ShouldRunDedicatedServerInitialSave(
		worldBootstrapPlan,
		IsShutdownRequested(),
		app.m_bShutdown))
	{
		// Windows64 suppresses saveToDisc right after new world creation
		// Dedicated Server explicitly runs the initial save here
		LogWorldIO("requesting initial save for newly created world");
		WaitForWorldActionIdle(kServerActionPad, 5000, &TickCoreSystems, &HandleXuiActions);
		app.SetXuiServerAction(kServerActionPad, eXuiServerAction_AutoSaveGame);
		if (!WaitForWorldActionIdle(kServerActionPad, 30000, &TickCoreSystems, &HandleXuiActions))
		{
			LogWorldIO("initial save timed out");
			LogWarn("world-io", "Timed out waiting for initial save action to finish.");
		}
		else
		{
			LogWorldIO("initial save completed");
		}
	}
	ServerRuntime::DedicatedServerAutosaveState autosaveState =
		ServerRuntime::CreateDedicatedServerAutosaveState(
			LceGetMonotonicMilliseconds(),
			serverProperties);
	ServerRuntime::ServerCli serverCli;
	serverCli.Start();

	while (!IsShutdownRequested() && !app.m_bShutdown)
	{
		TickCoreSystems();
		HandleXuiActions();
		serverCli.Poll();

		if (IsShutdownRequested() || app.m_bShutdown)
		{
			break;
		}

		const bool autosaveActionIdle =
			app.GetXuiServerAction(kServerActionPad) ==
				eXuiServerAction_Idle;
		const std::uint64_t now = LceGetMonotonicMilliseconds();
		const ServerRuntime::DedicatedServerAutosaveLoopPlan autosaveLoopPlan =
			ServerRuntime::BuildDedicatedServerAutosaveLoopPlan(
				autosaveState,
				autosaveActionIdle,
				now);

		if (autosaveLoopPlan.shouldMarkCompleted)
		{
			LogWorldIO("autosave completed");
			ServerRuntime::MarkDedicatedServerAutosaveCompleted(&autosaveState);
		}

		if (MinecraftServer::serverHalted())
		{
			break;
		}

		if (autosaveLoopPlan.shouldRequestAutosave)
		{
			LogWorldIO("requesting autosave");
			app.SetXuiServerAction(kServerActionPad, eXuiServerAction_AutoSaveGame);
			ServerRuntime::MarkDedicatedServerAutosaveRequested(
				&autosaveState,
				now,
				serverProperties);
		}
		else if (autosaveLoopPlan.shouldAdvanceDeadline)
		{
			autosaveState.nextTickMs =
				ServerRuntime::ComputeNextDedicatedServerAutosaveDeadlineMs(
					now,
					serverProperties);
		}

		LceSleepMilliseconds(10);
	}
	serverCli.Stop();
	app.m_bShutdown = true;

	LogInfof("shutdown", "Dedicated server stopped");
	MinecraftServer *server = MinecraftServer::getInstance();
	const ServerRuntime::DedicatedServerShutdownPlan shutdownPlan =
		ServerRuntime::BuildDedicatedServerShutdownPlan(
			server != NULL,
			g_NetworkManager.ServerStoppedValid());
	if (shutdownPlan.shouldSetSaveOnExit)
	{
		server->setSaveOnExit(true);
	}
	if (shutdownPlan.shouldLogShutdownSave)
	{
		LogWorldIO("requesting save before shutdown");
		LogWorldIO("using saveOnExit for shutdown");
	}

	MinecraftServer::HaltServer();

	if (shutdownPlan.shouldWaitForServerStop)
	{
		C4JThread waitThread(&WaitForServerStoppedThreadProc, NULL, "WaitServerStopped");
		waitThread.Run();
		waitThread.WaitForCompletion(INFINITE);
	}

	LogInfof("shutdown", "Cleaning up and exiting.");
	WinsockNetLayer::Shutdown();
	LogDebugf("shutdown", "Network layer shutdown complete.");
	g_NetworkManager.Terminate();
	LogDebugf("shutdown", "Network manager terminated.");
	ServerRuntime::ServerLogManager::Shutdown();
	CleanupDevice();
	

	return 0;
}
