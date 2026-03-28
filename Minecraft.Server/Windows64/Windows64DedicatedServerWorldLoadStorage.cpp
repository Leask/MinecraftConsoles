#include "stdafx.h"

#include "..\Common\DedicatedServerWorldLoadStorage.h"

#include "..\ServerLogger.h"
#include "..\Common\StringUtils.h"

#include <lce_time/lce_time.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
    using ServerRuntime::StringUtils::Utf8ToWide;
    using ServerRuntime::StringUtils::WideToUtf8;

    struct Windows64DedicatedServerWorldLoadStorageContext
    {
        bool queryDone = false;
        bool querySuccess = false;
        void *queryDetails = nullptr;
        bool loadDone = false;
        bool loadIsCorrupt = true;
        bool loadIsOwner = false;
        void *matchedSaveInfo = nullptr;
        int loadState = C4JStorage::ESaveGame_Idle;
    };

    static void DestroyWindows64DedicatedServerWorldLoadStorageContext(
        void *context)
    {
        delete static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
            context);
    }

    static void SetStorageSaveUniqueFilename(const std::string &saveFilename)
    {
        if (saveFilename.empty())
        {
            return;
        }

        char filenameBuffer[64] = {};
        strncpy_s(
            filenameBuffer,
            sizeof(filenameBuffer),
            saveFilename.c_str(),
            _TRUNCATE);
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

    static void LogEnumeratedSaveInfo(int index, const SAVE_INFO &saveInfo)
    {
        const std::wstring title = Utf8ToWide(saveInfo.UTF8SaveTitle);
        const std::wstring filename = Utf8ToWide(saveInfo.UTF8SaveFilename);
        const std::string titleUtf8 = WideToUtf8(title);
        const std::string filenameUtf8 = WideToUtf8(filename);

        char logLine[512] = {};
        sprintf_s(
            logLine,
            sizeof(logLine),
            "save[%d] title=\"%s\" filename=\"%s\"",
            index,
            titleUtf8.c_str(),
            filenameUtf8.c_str());
        ServerRuntime::LogDebug("world-io", logLine);
    }

    static int GetSavesInfoCallbackProc(
        LPVOID lpParam,
        SAVE_DETAILS *pSaveDetails,
        const bool bRes)
    {
        Windows64DedicatedServerWorldLoadStorageContext *context =
            static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
                lpParam);
        if (context != NULL)
        {
            context->queryDetails = pSaveDetails;
            context->querySuccess = bRes;
            context->queryDone = true;
        }
        return 0;
    }

    static int LoadSaveDataCallbackProc(
        LPVOID lpParam,
        const bool bIsCorrupt,
        const bool bIsOwner)
    {
        Windows64DedicatedServerWorldLoadStorageContext *context =
            static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
                lpParam);
        if (context != NULL)
        {
            context->loadIsCorrupt = bIsCorrupt;
            context->loadIsOwner = bIsOwner;
            context->loadDone = true;
        }
        return 0;
    }

    static bool WaitForSaveInfoResult(
        Windows64DedicatedServerWorldLoadStorageContext *context,
        DWORD timeoutMs,
        ServerRuntime::DedicatedServerWorldLoadTickProc tickProc)
    {
        const std::uint64_t start = LceGetMonotonicMilliseconds();
        while ((LceGetMonotonicMilliseconds() - start) < timeoutMs)
        {
            if (context->queryDone)
            {
                return true;
            }

            if (context->queryDetails == NULL)
            {
                SAVE_DETAILS *details = StorageManager.ReturnSavesInfo();
                if (details != NULL)
                {
                    context->queryDetails = details;
                    context->querySuccess = true;
                    context->queryDone = true;
                    return true;
                }
            }

            if (tickProc != NULL)
            {
                tickProc();
            }
            LceSleepMilliseconds(10);
        }

        return context->queryDone;
    }

    static bool WaitForSaveLoadResult(
        Windows64DedicatedServerWorldLoadStorageContext *context,
        DWORD timeoutMs,
        ServerRuntime::DedicatedServerWorldLoadTickProc tickProc)
    {
        const std::uint64_t start = LceGetMonotonicMilliseconds();
        while ((LceGetMonotonicMilliseconds() - start) < timeoutMs)
        {
            if (context->loadDone)
            {
                return true;
            }

            if (tickProc != NULL)
            {
                tickProc();
            }
            LceSleepMilliseconds(10);
        }

        return context->loadDone;
    }

    static bool ClearDedicatedServerWorldSaveQuery(void *context)
    {
        Windows64DedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == NULL)
        {
            return false;
        }

        StorageManager.ClearSavesInfo();
        storageContext->queryDone = false;
        storageContext->querySuccess = false;
        storageContext->queryDetails = NULL;
        storageContext->loadDone = false;
        storageContext->loadIsCorrupt = true;
        storageContext->loadIsOwner = false;
        storageContext->matchedSaveInfo = NULL;
        storageContext->loadState = C4JStorage::ESaveGame_Idle;
        return true;
    }

    static bool BeginDedicatedServerWorldSaveQuery(int actionPad, void *context)
    {
        Windows64DedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == NULL)
        {
            return false;
        }

        const int infoState = StorageManager.GetSavesInfo(
            actionPad,
            &GetSavesInfoCallbackProc,
            storageContext,
            "save");
        if (infoState == C4JStorage::ESaveGame_Idle)
        {
            storageContext->queryDone = true;
            storageContext->querySuccess = true;
            storageContext->queryDetails = StorageManager.ReturnSavesInfo();
            return true;
        }

        return infoState == C4JStorage::ESaveGame_GetSavesInfo;
    }

    static bool PollDedicatedServerWorldSaveQuery(
        DWORD timeoutMs,
        ServerRuntime::DedicatedServerWorldLoadTickProc tickProc,
        void *context,
        std::vector<ServerRuntime::DedicatedServerWorldLoadCandidate>
            *outCandidates)
    {
        Windows64DedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == NULL || outCandidates == NULL)
        {
            return false;
        }

        if (!WaitForSaveInfoResult(storageContext, timeoutMs, tickProc))
        {
            return false;
        }

        SAVE_DETAILS *details =
            static_cast<SAVE_DETAILS *>(storageContext->queryDetails);
        if (details == NULL)
        {
            details = StorageManager.ReturnSavesInfo();
            storageContext->queryDetails = details;
        }
        if (details == NULL)
        {
            return false;
        }

        outCandidates->clear();
        outCandidates->reserve(details->iSaveC);
        for (int i = 0; i < details->iSaveC; ++i)
        {
            const SAVE_INFO &saveInfo = details->SaveInfoA[i];
            LogEnumeratedSaveInfo(i, saveInfo);

            ServerRuntime::DedicatedServerWorldLoadCandidate candidate = {};
            candidate.title = Utf8ToWide(saveInfo.UTF8SaveTitle);
            candidate.filename = Utf8ToWide(saveInfo.UTF8SaveFilename);
            outCandidates->push_back(candidate);
        }

        return true;
    }

    static bool BeginDedicatedServerWorldSaveLoad(int matchedIndex, void *context)
    {
        Windows64DedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
                context);
        SAVE_DETAILS *details = storageContext != NULL
            ? static_cast<SAVE_DETAILS *>(storageContext->queryDetails)
            : NULL;
        if (storageContext == NULL ||
            details == NULL ||
            matchedIndex < 0 ||
            matchedIndex >= details->iSaveC)
        {
            return false;
        }

        storageContext->matchedSaveInfo = &details->SaveInfoA[matchedIndex];
        storageContext->loadDone = false;
        storageContext->loadIsCorrupt = true;
        storageContext->loadIsOwner = false;
        storageContext->loadState = StorageManager.LoadSaveData(
            static_cast<SAVE_INFO *>(storageContext->matchedSaveInfo),
            &LoadSaveDataCallbackProc,
            storageContext);
        return storageContext->loadState == C4JStorage::ESaveGame_Load ||
            storageContext->loadState == C4JStorage::ESaveGame_Idle;
    }

    static bool PollDedicatedServerWorldSaveLoad(
        DWORD timeoutMs,
        ServerRuntime::DedicatedServerWorldLoadTickProc tickProc,
        void *context,
        bool *outIsCorrupt)
    {
        Windows64DedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<Windows64DedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == NULL || storageContext->matchedSaveInfo == NULL)
        {
            return false;
        }

        if (outIsCorrupt != NULL)
        {
            *outIsCorrupt = false;
        }

        if (storageContext->loadState == C4JStorage::ESaveGame_Load &&
            !WaitForSaveLoadResult(storageContext, timeoutMs, tickProc))
        {
            return false;
        }

        if (outIsCorrupt != NULL)
        {
            *outIsCorrupt = storageContext->loadIsCorrupt;
        }
        return true;
    }

    static bool ReadDedicatedServerWorldSaveBytes(
        void *,
        std::vector<unsigned char> *outBytes)
    {
        if (outBytes == NULL)
        {
            return false;
        }

        const unsigned int saveSize = StorageManager.GetSaveSize();
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
}

namespace ServerRuntime
{
    DedicatedServerWorldLoadStorageRuntime
    CreateDedicatedServerPlatformWorldLoadStorageRuntime()
    {
        DedicatedServerWorldLoadStorageRuntime runtime = {};
        Windows64DedicatedServerWorldLoadStorageContext *context =
            new Windows64DedicatedServerWorldLoadStorageContext();
        if (context == nullptr)
        {
            return runtime;
        }

        runtime.platformContext = context;
        runtime.destroyContextProc =
            &DestroyWindows64DedicatedServerWorldLoadStorageContext;
        runtime.storageHooks.setWorldTitleProc =
            &SetDedicatedServerWorldStorageTitle;
        runtime.storageHooks.setWorldSaveIdProc =
            &SetDedicatedServerWorldStorageSaveId;
        runtime.storageHooks.resetSaveDataProc =
            &ResetDedicatedServerWorldStorageData;
        runtime.loadHooks.clearSavesProc =
            &ClearDedicatedServerWorldSaveQuery;
        runtime.loadHooks.beginQueryProc =
            &BeginDedicatedServerWorldSaveQuery;
        runtime.loadHooks.pollQueryProc =
            &PollDedicatedServerWorldSaveQuery;
        runtime.loadHooks.beginLoadProc =
            &BeginDedicatedServerWorldSaveLoad;
        runtime.loadHooks.pollLoadProc =
            &PollDedicatedServerWorldSaveLoad;
        runtime.loadHooks.readBytesProc =
            &ReadDedicatedServerWorldSaveBytes;
        runtime.loadHooks.context = context;
        return runtime;
    }
}
