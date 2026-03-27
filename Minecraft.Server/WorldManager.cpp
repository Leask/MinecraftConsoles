#include "stdafx.h"

#include "WorldManager.h"

#include "Minecraft.h"
#include "MinecraftServer.h"
#include "ServerLogger.h"
#include "Common\\DedicatedServerWorldBootstrap.h"
#include "Common\\ServerStoragePaths.h"
#include "Common\\StringUtils.h"
#include "Common\\DedicatedServerWorldSaveSelection.h"
#include <lce_filesystem/lce_filesystem.h>
#include <lce_time/lce_time.h>

#include <stdio.h>
#include <string.h>
#include <vector>

namespace ServerRuntime
{
using StringUtils::Utf8ToWide;
using StringUtils::WideToUtf8;

struct SaveInfoQueryContext
{
	bool done;
	bool success;
	SAVE_DETAILS *details;

	SaveInfoQueryContext()
		: done(false)
		, success(false)
		, details(NULL)
	{
	}
};

struct SaveDataLoadContext
{
	bool done;
	bool isCorrupt;
	bool isOwner;

	SaveDataLoadContext()
		: done(false)
		, isCorrupt(true)
		, isOwner(false)
	{
	}
};

/**
 * **Apply Save ID To StorageManager**
 *
 * Applies the configured save destination ID (`level-id`) to `StorageManager`
 * - Re-applies the same ID at startup and before save to avoid destination drift
 * - Ignores empty values as invalid
 * - For some reason, a date-based world file occasionally appears after a debug build, but the cause is unknown.
 * 保存先IDの適用処理
 *
 * @param saveFilename Normalized save destination ID
 */
static void SetStorageSaveUniqueFilename(const std::string &saveFilename)
{
	if (saveFilename.empty())
	{
		return;
	}

	char filenameBuffer[64] = {};
	strncpy_s(filenameBuffer, sizeof(filenameBuffer), saveFilename.c_str(), _TRUNCATE);
	StorageManager.SetSaveUniqueFilename(filenameBuffer);
}

static void LogSaveFilename(const char *prefix, const std::string &saveFilename)
{
	LogInfof("world-io", "%s: %s", (prefix != NULL) ? prefix : "save-filename", saveFilename.c_str());
}

/**
 * Verifies a directory exists and creates it when missing
 * - Treats an existing non-directory path as failure
 * - Returns whether the directory had to be created
 * ディレクトリ存在保証の補助処理
 */
static bool EnsureDirectoryExists(const std::string &directoryPath, bool *outCreated)
{
	if (outCreated != NULL)
	{
		*outCreated = false;
	}
	if (directoryPath.empty())
	{
		return false;
	}

	if (DirectoryExists(directoryPath.c_str()))
	{
		return true;
	}

	if (FileExists(directoryPath.c_str()))
	{
		LogErrorf(
			"world-io",
			"path exists but is not a directory: %s",
			directoryPath.c_str());
		return false;
	}

	if (CreateDirectoryIfMissing(directoryPath.c_str(), outCreated))
	{
		return true;
	}

	LogErrorf(
		"world-io",
		"failed to create directory %s",
		directoryPath.c_str());
	return false;
}

/**
 * Prepares the save root used by the current platform storage layout
 * - Creates the platform root first because the directory helper is not recursive
 * - Creates `{platform}/GameHDD` when missing before world bootstrap starts
 * プラットフォーム保存先ディレクトリの存在保証
 */
static bool EnsureGameHddRootExists()
{
	const std::string platformDirectory =
		ServerRuntime::GetServerStoragePlatformDirectory();
	const std::string gameHddRoot =
		ServerRuntime::GetServerGameHddRootPath();

	bool windows64Created = false;
	if (!EnsureDirectoryExists(platformDirectory, &windows64Created))
	{
		return false;
	}

	bool gameHddCreated = false;
	if (!EnsureDirectoryExists(gameHddRoot, &gameHddCreated))
	{
		return false;
	}

	if (windows64Created || gameHddCreated)
	{
		LogInfof(
			"world-io",
			"created missing %s storage directories",
			gameHddRoot.c_str());
	}

	return true;
}

static void LogEnumeratedSaveInfo(int index, const SAVE_INFO &saveInfo)
{
	std::wstring title = Utf8ToWide(saveInfo.UTF8SaveTitle);
	std::wstring filename = Utf8ToWide(saveInfo.UTF8SaveFilename);
	std::string titleUtf8 = WideToUtf8(title);
	std::string filenameUtf8 = WideToUtf8(filename);

	char logLine[512] = {};
	sprintf_s(
		logLine,
		sizeof(logLine),
		"save[%d] title=\"%s\" filename=\"%s\"",
		index,
		titleUtf8.c_str(),
		filenameUtf8.c_str());
	LogDebug("world-io", logLine);
}

/**
 * **Save List Callback**
 *
 * Captures async save-list results into `SaveInfoQueryContext` and marks completion for the waiter
 * セーブ一覧取得の完了通知
 */
static int GetSavesInfoCallbackProc(LPVOID lpParam, SAVE_DETAILS *pSaveDetails, const bool bRes)
{
	SaveInfoQueryContext *context = (SaveInfoQueryContext *)lpParam;
	if (context != NULL)
	{
		context->details = pSaveDetails;
		context->success = bRes;
		context->done = true;
	}
	return 0;
}

/**
 * **Save Data Load Callback**
 *
 * Writes load results such as corruption status into `SaveDataLoadContext`
 * セーブ読み込み結果の反映
 */
static int LoadSaveDataCallbackProc(LPVOID lpParam, const bool bIsCorrupt, const bool bIsOwner)
{
	SaveDataLoadContext *context = (SaveDataLoadContext *)lpParam;
	if (context != NULL)
	{
		context->isCorrupt = bIsCorrupt;
		context->isOwner = bIsOwner;
		context->done = true;
	}
	return 0;
}

/**
 * **Wait For Save List Completion**
 *
 * Waits until save-list retrieval completes
 * - Prefers callback completion as the primary signal
 * - Also falls back to polling because some environments populate `ReturnSavesInfo()` before callback
 * セーブ一覧待機の補助処理
 *
 * @return `true` when completion is detected
 */
static bool WaitForSaveInfoResult(SaveInfoQueryContext *context, DWORD timeoutMs, WorldManagerTickProc tickProc)
{
	const std::uint64_t start = LceGetMonotonicMilliseconds();
	while ((LceGetMonotonicMilliseconds() - start) < timeoutMs)
	{
		if (context->done)
		{
			return true;
		}

		if (context->details == NULL)
		{
			// Some implementations fill ReturnSavesInfo before the callback
			// Keep polling as a fallback instead of relying only on callback completion
			SAVE_DETAILS *details = StorageManager.ReturnSavesInfo();
			if (details != NULL)
			{
				context->details = details;
				context->success = true;
				context->done = true;
				return true;
			}
		}

		if (tickProc != NULL)
		{
			tickProc();
		}
		LceSleepMilliseconds(10);
	}

	return context->done;
}

/**
 * **Wait For Save Data Load Completion**
 *
 * Waits for the save-data load callback to complete
 * セーブ本体の読み込み待機
 *
 * @return `true` when callback is reached, `false` on timeout
 */
static bool WaitForSaveLoadResult(SaveDataLoadContext *context, DWORD timeoutMs, WorldManagerTickProc tickProc)
{
	const std::uint64_t start = LceGetMonotonicMilliseconds();
	while ((LceGetMonotonicMilliseconds() - start) < timeoutMs)
	{
		if (context->done)
		{
			return true;
		}

		if (tickProc != NULL)
		{
			tickProc();
		}
		LceSleepMilliseconds(10);
	}

	return context->done;
}

/**
 * **Apply World Identity To Storage**
 *
 * Applies world identity (`level-name` + `level-id`) to storage
 * - Always sets both display name and ID to avoid partial configuration
 * - Helps prevent unintended new save destinations across environment differences
 * 保存先と表示名の同期処理
 */
static void ApplyWorldStorageTarget(const std::wstring &worldName, const std::string &saveId)
{
	// Set both title (display name) and save ID (actual folder name) explicitly
	// Setting only one side can create unexpected new save targets in some environments
	StorageManager.SetSaveTitle(worldName.c_str());
	SetStorageSaveUniqueFilename(saveId);
}

/**
 * **Prepare World Save Data For Startup**
 *
 * Searches for a save matching the target world and extracts startup payload when found
 * Match priority:
 * 1. Exact match by `level-id` (`UTF8SaveFilename`)
 * 2. Fallback match by `level-name` against title or filename
 * ワールド一致セーブの探索処理
 *
 * @return
 * - `eWorldSaveLoad_Loaded`: Existing save loaded successfully
 * - `eWorldSaveLoad_NotFound`: No matching save found
 * - `eWorldSaveLoad_Failed`: API failure, corruption, or invalid data
 */
static EDedicatedServerWorldLoadStatus PrepareWorldSaveData(
	const std::wstring &targetWorldName,
	const std::string &targetSaveFilename,
	int actionPad,
	WorldManagerTickProc tickProc,
	LoadSaveDataThreadParam **outSaveData,
	std::string *outResolvedSaveFilename)
{
	if (outSaveData == NULL)
	{
		return eDedicatedServerWorldLoad_Failed;
	}
	*outSaveData = NULL;
	if (outResolvedSaveFilename != NULL)
	{
		outResolvedSaveFilename->clear();
	}

	LogWorldIO("enumerating saves for configured world");
	StorageManager.ClearSavesInfo();

	SaveInfoQueryContext infoContext;
	int infoState = StorageManager.GetSavesInfo(actionPad, &GetSavesInfoCallbackProc, &infoContext, "save");
	if (infoState == C4JStorage::ESaveGame_Idle)
	{
		infoContext.done = true;
		infoContext.success = true;
		infoContext.details = StorageManager.ReturnSavesInfo();
	}
	else if (infoState != C4JStorage::ESaveGame_GetSavesInfo)
	{
		LogWorldIO("GetSavesInfo failed to start");
		return eDedicatedServerWorldLoad_Failed;
	}

	if (!WaitForSaveInfoResult(&infoContext, 10000, tickProc))
	{
		LogWorldIO("timed out waiting for save list");
		return eDedicatedServerWorldLoad_Failed;
	}

	if (infoContext.details == NULL)
	{
		infoContext.details = StorageManager.ReturnSavesInfo();
	}
	if (infoContext.details == NULL)
	{
		LogWorldIO("failed to retrieve save list");
		return eDedicatedServerWorldLoad_Failed;
	}

	std::vector<ServerRuntime::DedicatedServerWorldSaveMatchEntry> saveEntries;
	saveEntries.reserve(infoContext.details->iSaveC);
	for (int i = 0; i < infoContext.details->iSaveC; ++i)
	{
		const SAVE_INFO &saveInfo = infoContext.details->SaveInfoA[i];
		LogEnumeratedSaveInfo(i, saveInfo);

		ServerRuntime::DedicatedServerWorldSaveMatchEntry matchEntry = {};
		matchEntry.title = Utf8ToWide(saveInfo.UTF8SaveTitle);
		matchEntry.filename = Utf8ToWide(saveInfo.UTF8SaveFilename);
		saveEntries.push_back(matchEntry);
	}

	const ServerRuntime::DedicatedServerWorldSaveSelectionResult
		saveSelection =
			ServerRuntime::SelectDedicatedServerWorldSaveEntry(
				saveEntries.empty() ? NULL : &saveEntries[0],
				static_cast<int>(saveEntries.size()),
				targetWorldName,
				targetSaveFilename);
	const int matchedIndex = saveSelection.matchedIndex;

	if (matchedIndex < 0)
	{
		LogWorldIO("no save matched configured world name");
		return eDedicatedServerWorldLoad_NotFound;
	}

	std::wstring matchedTitle = Utf8ToWide(infoContext.details->SaveInfoA[matchedIndex].UTF8SaveTitle);
	if (matchedTitle.empty())
	{
		matchedTitle = targetWorldName;
	}
	LogWorldName("matched save title", matchedTitle);
	SAVE_INFO *matchedSaveInfo = &infoContext.details->SaveInfoA[matchedIndex];
	std::wstring matchedFilename = Utf8ToWide(matchedSaveInfo->UTF8SaveFilename);
	if (!matchedFilename.empty())
	{
		LogWorldName("matched save filename", matchedFilename);
	}

	ApplyWorldStorageTarget(targetWorldName, targetSaveFilename);

	std::string resolvedSaveFilename;
	if (matchedSaveInfo->UTF8SaveFilename[0] != 0)
	{
		// Prefer the save ID that was actually matched, then keep using it for future saves
		resolvedSaveFilename = matchedSaveInfo->UTF8SaveFilename;
		SetStorageSaveUniqueFilename(resolvedSaveFilename);
	}
	else if (!targetSaveFilename.empty())
	{
		resolvedSaveFilename = targetSaveFilename;
	}

	if (outResolvedSaveFilename != NULL)
	{
		*outResolvedSaveFilename = resolvedSaveFilename;
	}

	SaveDataLoadContext loadContext;
	int loadState = StorageManager.LoadSaveData(matchedSaveInfo, &LoadSaveDataCallbackProc, &loadContext);
	if (loadState != C4JStorage::ESaveGame_Load && loadState != C4JStorage::ESaveGame_Idle)
	{
		LogWorldIO("LoadSaveData failed to start");
		return eDedicatedServerWorldLoad_Failed;
	}

	if (loadState == C4JStorage::ESaveGame_Load)
	{
		if (!WaitForSaveLoadResult(&loadContext, 15000, tickProc))
		{
			LogWorldIO("timed out waiting for save data load");
			return eDedicatedServerWorldLoad_Failed;
		}
		if (loadContext.isCorrupt)
		{
			LogWorldIO("target save is corrupt; aborting load");
			return eDedicatedServerWorldLoad_Failed;
		}
	}

	unsigned int saveSize = StorageManager.GetSaveSize();
	if (saveSize == 0)
	{
		// Treat zero-byte payload as failure even when load API reports success
		LogWorldIO("loaded save has zero size");
		return eDedicatedServerWorldLoad_Failed;
	}

	byteArray loadedSaveData(saveSize, false);
	unsigned int loadedSize = saveSize;
	StorageManager.GetSaveData(loadedSaveData.data, &loadedSize);
	if (loadedSize == 0)
	{
		LogWorldIO("failed to copy loaded save data from storage manager");
		return eDedicatedServerWorldLoad_Failed;
	}

	*outSaveData = new LoadSaveDataThreadParam(loadedSaveData.data, loadedSize, matchedTitle);
	LogWorldIO("prepared save data payload for server startup");
	return eDedicatedServerWorldLoad_Loaded;
}

/**
 * **Bootstrap World State For Server Startup**
 *
 * Determines final world startup state
 * - Returns loaded save data when an existing save is found
 * - Prepares a new world context when not found
 * - Returns `Failed` when startup should be aborted
 * サーバー起動時のワールド確定処理
 */
WorldBootstrapResult BootstrapWorldForServer(
	const ServerPropertiesConfig &config,
	int actionPad,
	WorldManagerTickProc tickProc)
{
	WorldBootstrapResult result;
	if (!EnsureGameHddRootExists())
	{
		LogErrorf(
			"world-io",
			"failed to prepare %s storage root",
			GetServerGameHddRootPath().c_str());
		return result;
	}

	const DedicatedServerWorldTarget worldTarget =
		ResolveDedicatedServerWorldTarget(config);
	const std::wstring &targetWorldName = worldTarget.worldName;
	const std::string &targetSaveFilename = worldTarget.saveId;

	LogWorldName("configured level-name", targetWorldName);
	if (!targetSaveFilename.empty())
	{
		LogSaveFilename("configured level-id", targetSaveFilename);
	}

	ApplyWorldStorageTarget(targetWorldName, targetSaveFilename);

	LoadSaveDataThreadParam *loadedSaveData = NULL;
	std::string loadedSaveFilename;
	const EDedicatedServerWorldLoadStatus worldLoadResult =
		PrepareWorldSaveData(
		targetWorldName,
		targetSaveFilename,
		actionPad,
		tickProc,
		&loadedSaveData,
		&loadedSaveFilename);
	result = BuildDedicatedServerWorldBootstrapResult(
		worldLoadResult,
		loadedSaveData,
		worldTarget,
		loadedSaveFilename);
	if (result.status == eWorldBootstrap_Loaded)
	{
		LogStartupStep("loading configured world from save data");
	}
	else if (result.status == eWorldBootstrap_CreatedNew)
	{
		// Create a new context only when no matching save exists
		// Fix saveId here so the next startup writes to the same location
		LogStartupStep("configured world not found; creating new world");
		LogWorldIO("creating new world save context");
		StorageManager.ResetSaveData();
		ApplyWorldStorageTarget(targetWorldName, targetSaveFilename);
	}

	return result;
}

/**
 * **Wait Until Server XUI Action Is Idle**
 *
 * Keeps tick/handle running during save action so async processing does not stall
 * XUIアクション待機中の進行維持処理
 */
bool WaitForWorldActionIdle(
	int actionPad,
	DWORD timeoutMs,
	WorldManagerTickProc tickProc,
	WorldManagerHandleActionsProc handleActionsProc)
{
	const std::uint64_t start = LceGetMonotonicMilliseconds();
	while (app.GetXuiServerAction(actionPad) != eXuiServerAction_Idle && !MinecraftServer::serverHalted())
	{
		// Keep network and storage progressing while waiting
		// If this stops, save action itself may stall and time out
		if (tickProc != NULL)
		{
			tickProc();
		}
		if (handleActionsProc != NULL)
		{
			handleActionsProc();
		}
		if ((LceGetMonotonicMilliseconds() - start) >= timeoutMs)
		{
			return false;
		}
		LceSleepMilliseconds(10);
	}

	return (app.GetXuiServerAction(actionPad) == eXuiServerAction_Idle);
}
}
