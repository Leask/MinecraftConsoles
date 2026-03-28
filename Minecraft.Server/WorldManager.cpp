#include "stdafx.h"

#include "WorldManager.h"

#include "ServerLogger.h"
#include "Common\\DedicatedServerWorldBootstrap.h"
#include "Common\\DedicatedServerWorldLoadPipeline.h"
#include "Common\\ServerStoragePaths.h"
#include "Common\\StringUtils.h"
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

struct WorldLoadStorageContext
{
	SaveInfoQueryContext saveInfo = {};
	SaveDataLoadContext saveDataLoad = {};
	SAVE_INFO *matchedSaveInfo = NULL;
	int loadState = C4JStorage::ESaveGame_Idle;
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

static void SetDedicatedServerWorldStorageTitle(
	const std::wstring &worldName,
	void *)
{
	StorageManager.SetSaveTitle(worldName.c_str());
}

static void SetDedicatedServerWorldStorageSaveId(
	const std::string &saveId,
	void *)
{
	SetStorageSaveUniqueFilename(saveId);
}

static void ResetDedicatedServerWorldStorageData(void *)
{
	StorageManager.ResetSaveData();
}

static DedicatedServerWorldStorageHooks BuildDedicatedServerWorldStorageHooks()
{
	DedicatedServerWorldStorageHooks hooks = {};
	hooks.setWorldTitleProc = &SetDedicatedServerWorldStorageTitle;
	hooks.setWorldSaveIdProc = &SetDedicatedServerWorldStorageSaveId;
	hooks.resetSaveDataProc = &ResetDedicatedServerWorldStorageData;
	return hooks;
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

static bool ClearDedicatedServerWorldSaveQuery(void *context)
{
	WorldLoadStorageContext *storageContext =
		static_cast<WorldLoadStorageContext *>(context);
	if (storageContext == NULL)
	{
		return false;
	}

	StorageManager.ClearSavesInfo();
	storageContext->saveInfo = SaveInfoQueryContext();
	storageContext->saveDataLoad = SaveDataLoadContext();
	storageContext->matchedSaveInfo = NULL;
	storageContext->loadState = C4JStorage::ESaveGame_Idle;
	return true;
}

static bool BeginDedicatedServerWorldSaveQuery(int actionPad, void *context)
{
	WorldLoadStorageContext *storageContext =
		static_cast<WorldLoadStorageContext *>(context);
	if (storageContext == NULL)
	{
		return false;
	}

	int infoState = StorageManager.GetSavesInfo(
		actionPad,
		&GetSavesInfoCallbackProc,
		&storageContext->saveInfo,
		"save");
	if (infoState == C4JStorage::ESaveGame_Idle)
	{
		storageContext->saveInfo.done = true;
		storageContext->saveInfo.success = true;
		storageContext->saveInfo.details = StorageManager.ReturnSavesInfo();
		return true;
	}

	return infoState == C4JStorage::ESaveGame_GetSavesInfo;
}

static bool PollDedicatedServerWorldSaveQuery(
	DWORD timeoutMs,
	DedicatedServerWorldLoadTickProc tickProc,
	void *context,
	std::vector<DedicatedServerWorldLoadCandidate> *outCandidates)
{
	WorldLoadStorageContext *storageContext =
		static_cast<WorldLoadStorageContext *>(context);
	if (storageContext == NULL || outCandidates == NULL)
	{
		return false;
	}

	if (!WaitForSaveInfoResult(&storageContext->saveInfo, timeoutMs, tickProc))
	{
		return false;
	}

	if (storageContext->saveInfo.details == NULL)
	{
		storageContext->saveInfo.details = StorageManager.ReturnSavesInfo();
	}
	if (storageContext->saveInfo.details == NULL)
	{
		return false;
	}

	outCandidates->clear();
	outCandidates->reserve(storageContext->saveInfo.details->iSaveC);
	for (int i = 0; i < storageContext->saveInfo.details->iSaveC; ++i)
	{
		const SAVE_INFO &saveInfo = storageContext->saveInfo.details->SaveInfoA[i];
		LogEnumeratedSaveInfo(i, saveInfo);

		DedicatedServerWorldLoadCandidate candidate = {};
		candidate.title = Utf8ToWide(saveInfo.UTF8SaveTitle);
		candidate.filename = Utf8ToWide(saveInfo.UTF8SaveFilename);
		outCandidates->push_back(candidate);
	}

	return true;
}

static bool BeginDedicatedServerWorldSaveLoad(int matchedIndex, void *context)
{
	WorldLoadStorageContext *storageContext =
		static_cast<WorldLoadStorageContext *>(context);
	if (storageContext == NULL ||
		storageContext->saveInfo.details == NULL ||
		matchedIndex < 0 ||
		matchedIndex >= storageContext->saveInfo.details->iSaveC)
	{
		return false;
	}

	storageContext->matchedSaveInfo =
		&storageContext->saveInfo.details->SaveInfoA[matchedIndex];
	storageContext->saveDataLoad = SaveDataLoadContext();
	storageContext->loadState = StorageManager.LoadSaveData(
		storageContext->matchedSaveInfo,
		&LoadSaveDataCallbackProc,
		&storageContext->saveDataLoad);
	return storageContext->loadState == C4JStorage::ESaveGame_Load ||
		storageContext->loadState == C4JStorage::ESaveGame_Idle;
}

static bool PollDedicatedServerWorldSaveLoad(
	DWORD timeoutMs,
	DedicatedServerWorldLoadTickProc tickProc,
	void *context,
	bool *outIsCorrupt)
{
	WorldLoadStorageContext *storageContext =
		static_cast<WorldLoadStorageContext *>(context);
	if (storageContext == NULL || storageContext->matchedSaveInfo == NULL)
	{
		return false;
	}

	if (outIsCorrupt != NULL)
	{
		*outIsCorrupt = false;
	}

	if (storageContext->loadState == C4JStorage::ESaveGame_Load)
	{
		if (!WaitForSaveLoadResult(
				&storageContext->saveDataLoad,
				timeoutMs,
				tickProc))
		{
			return false;
		}
		if (outIsCorrupt != NULL)
		{
			*outIsCorrupt = storageContext->saveDataLoad.isCorrupt;
		}
	}

	return true;
}

static bool ReadDedicatedServerWorldSaveBytes(
	void *context,
	std::vector<unsigned char> *outBytes)
{
	if (context == NULL || outBytes == NULL)
	{
		return false;
	}

	unsigned int saveSize = StorageManager.GetSaveSize();
	if (saveSize == 0)
	{
		return false;
	}

	outBytes->assign(saveSize, 0);
	unsigned int loadedSize = saveSize;
	StorageManager.GetSaveData(&(*outBytes)[0], &loadedSize);
	if (loadedSize == 0)
	{
		outBytes->clear();
		return false;
	}

	outBytes->resize(loadedSize);
	return true;
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
	const DedicatedServerWorldTarget worldTarget = {
		targetWorldName,
		targetSaveFilename
	};
	WorldLoadStorageContext storageContext = {};
	DedicatedServerWorldLoadHooks loadHooks = {};
	loadHooks.clearSavesProc = &ClearDedicatedServerWorldSaveQuery;
	loadHooks.beginQueryProc = &BeginDedicatedServerWorldSaveQuery;
	loadHooks.pollQueryProc = &PollDedicatedServerWorldSaveQuery;
	loadHooks.beginLoadProc = &BeginDedicatedServerWorldSaveLoad;
	loadHooks.pollLoadProc = &PollDedicatedServerWorldSaveLoad;
	loadHooks.readBytesProc = &ReadDedicatedServerWorldSaveBytes;
	loadHooks.context = &storageContext;

	DedicatedServerWorldLoadPayload loadPayload = {};
	const EDedicatedServerWorldLoadStatus loadStatus =
		LoadDedicatedServerWorldData(
			worldTarget,
			actionPad,
			tickProc,
			loadHooks,
			&loadPayload);
	if (loadStatus != eDedicatedServerWorldLoad_Loaded)
	{
		if (loadStatus == eDedicatedServerWorldLoad_NotFound)
		{
			LogWorldIO("no save matched configured world name");
		}
		else
		{
			LogWorldIO("failed to load matched world save");
		}
		return loadStatus;
	}

	LogWorldName("matched save title", loadPayload.matchedTitle);
	if (!loadPayload.matchedFilename.empty())
	{
		LogWorldName("matched save filename", loadPayload.matchedFilename);
	}

	if (outResolvedSaveFilename != NULL)
	{
		*outResolvedSaveFilename = loadPayload.resolvedSaveId;
	}

	ApplyDedicatedServerWorldStoragePlan(
		BuildLoadedDedicatedServerWorldStoragePlan(
			worldTarget,
			loadPayload.resolvedSaveId),
		BuildDedicatedServerWorldStorageHooks());

	*outSaveData = new LoadSaveDataThreadParam(
		loadPayload.bytes.data(),
		static_cast<unsigned int>(loadPayload.bytes.size()),
		loadPayload.matchedTitle);
	LogWorldIO("prepared save data payload for server startup");
	return loadStatus;
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
	const DedicatedServerWorldStorageHooks storageHooks =
		BuildDedicatedServerWorldStorageHooks();

	LogWorldName("configured level-name", targetWorldName);
	if (!targetSaveFilename.empty())
	{
		LogSaveFilename("configured level-id", targetSaveFilename);
	}

	ApplyDedicatedServerWorldStoragePlan(
		BuildConfiguredDedicatedServerWorldStoragePlan(worldTarget),
		storageHooks);

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
		ApplyDedicatedServerWorldStoragePlan(
			BuildCreatedDedicatedServerWorldStoragePlan(worldTarget),
			storageHooks);
	}

	return result;
}
}
