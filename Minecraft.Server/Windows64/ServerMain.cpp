#include "stdafx.h"

#include "Common/App_Defines.h"
#include "Common/Network/GameNetworkManager.h"
#include "Input.h"
#include "Minecraft.h"
#include "MinecraftServer.h"
#include "..\Common\DedicatedServerBootstrap.h"
#include "..\Common\DedicatedServerOptions.h"
#include "..\Common\DedicatedServerPlatformState.h"
#include "..\Common\DedicatedServerPlatformRuntime.h"
#include "..\Common\DedicatedServerRuntime.h"
#include "..\Common\DedicatedServerSignalHandlers.h"
#include "..\Common\DedicatedServerSignalState.h"
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
#include "Windows64/Windows64DedicatedServerRuntime.h"
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

#include <cwchar>

static const int kServerActionPad = 0;

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

class DedicatedServerLogManagerGuard
{
public:
	DedicatedServerLogManagerGuard()
		: m_active(false)
	{
	}

	void Activate()
	{
		m_active = true;
	}

	void Release()
	{
		m_active = false;
	}

	~DedicatedServerLogManagerGuard()
	{
		if (m_active)
		{
			ServerRuntime::ServerLogManager::Shutdown();
		}
	}

private:
	bool m_active;
};

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
	DedicatedServerLogManagerGuard logManagerGuard;
	ServerRuntime::ResetDedicatedServerShutdownRequest();

	if (!ServerRuntime::InstallDedicatedServerShutdownSignalHandlers())
	{
		LogWarn(
			"startup",
			"Failed to install dedicated server shutdown handlers.");
	}
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
	LogStartupStep("initializing server log manager");
	ServerRuntime::ServerLogManager::Initialize();
	logManagerGuard.Activate();
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

	const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
		windowsRuntimeStartResult =
			ServerRuntime::StartDedicatedServerPlatformRuntime(platformState);
	if (!windowsRuntimeStartResult.ok)
	{
		LogError("startup", windowsRuntimeStartResult.errorMessage.c_str());
		return windowsRuntimeStartResult.exitCode;
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
	WorldBootstrapResult worldBootstrap = BootstrapWorldForServer(
		serverProperties,
		kServerActionPad,
		&ServerRuntime::TickDedicatedServerPlatformRuntime);
	const ServerRuntime::DedicatedServerWorldBootstrapPlan worldBootstrapPlan =
		ServerRuntime::BuildDedicatedServerWorldBootstrapPlan(
			serverProperties,
			worldBootstrap);
	const ServerRuntime::DedicatedServerWorldLoadPlan worldLoadPlan =
		ServerRuntime::BuildDedicatedServerWorldLoadPlan(
			worldBootstrapPlan);
	if (worldLoadPlan.shouldPersistResolvedSaveId)
	{
		// Persist the actually loaded save-id back to config
		// Keep lookup keys aligned for next startup
		LogWorldIO("updating level-id to loaded save filename");
		serverProperties.worldSaveId = worldLoadPlan.resolvedSaveId;
		if (!SaveServerPropertiesConfig(serverProperties))
		{
			LogWorldIO("failed to persist updated level-id");
		}
	}
	else if (worldLoadPlan.shouldAbortStartup)
	{
		LogErrorf(
			"world-io",
			"Failed to load configured world \"%s\".",
			WideToUtf8(worldBootstrapPlan.targetWorldName).c_str());
		ServerRuntime::StopDedicatedServerPlatformRuntime();
		
		return worldLoadPlan.abortExitCode;
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

	while (startThread->isRunning() &&
		!ServerRuntime::IsDedicatedServerShutdownRequested())
	{
		ServerRuntime::TickDedicatedServerPlatformRuntime();
		LceSleepMilliseconds(10);
	}

	startThread->WaitForCompletion(INFINITE);
	int startupResult = startThread->GetExitCode();
	delete startThread;

	const ServerRuntime::DedicatedServerHostedThreadStartupPlan
		hostedThreadStartupPlan =
			ServerRuntime::BuildDedicatedServerHostedThreadStartupPlan(
				startupResult);
	if (hostedThreadStartupPlan.shouldAbortStartup)
	{
		LogErrorf("startup", "Failed to start dedicated server (code %d).", startupResult);
		ServerRuntime::StopDedicatedServerPlatformRuntime();
		
		return hostedThreadStartupPlan.abortExitCode;
	}

	LogStartupStep("server startup complete");
	LogInfof(
		"startup",
		"Dedicated server listening on %s:%d",
		platformState.bindIp.c_str(),
		platformState.multiplayerPort);
	const ServerRuntime::DedicatedServerInitialSavePlan initialSavePlan =
		ServerRuntime::BuildDedicatedServerInitialSavePlan(
		worldBootstrapPlan,
		ServerRuntime::IsDedicatedServerShutdownRequested(),
		app.m_bShutdown);
	if (initialSavePlan.shouldRequestInitialSave)
	{
		// Windows64 suppresses saveToDisc right after new world creation
		// Dedicated Server explicitly runs the initial save here
		LogWorldIO("requesting initial save for newly created world");
		ServerRuntime::WaitForDedicatedServerWorldActionIdle(
			kServerActionPad,
			initialSavePlan.idleWaitBeforeRequestMs);
		ServerRuntime::RequestDedicatedServerWorldAutosave(kServerActionPad);
		if (!ServerRuntime::WaitForDedicatedServerWorldActionIdle(
			kServerActionPad,
			initialSavePlan.requestTimeoutMs))
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

	while (!ServerRuntime::IsDedicatedServerShutdownRequested() &&
		!app.m_bShutdown)
	{
		ServerRuntime::TickDedicatedServerPlatformRuntime();
		ServerRuntime::HandleDedicatedServerPlatformActions();
		serverCli.Poll();

		if (ServerRuntime::IsDedicatedServerShutdownRequested() ||
			app.m_bShutdown)
		{
			break;
		}

		const bool autosaveActionIdle =
			ServerRuntime::IsDedicatedServerWorldActionIdle(
				kServerActionPad);
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
			ServerRuntime::RequestDedicatedServerWorldAutosave(
				kServerActionPad);
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
	ServerRuntime::StopDedicatedServerPlatformRuntime();
	logManagerGuard.Release();
	ServerRuntime::ServerLogManager::Shutdown();
	

	return 0;
}
