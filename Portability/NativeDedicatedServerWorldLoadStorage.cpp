#include "Minecraft.Server/Common/DedicatedServerWorldLoadStorage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "Minecraft.Server/Common/NativeDedicatedServerLoadedSaveState.h"
#include "Minecraft.Server/Common/ServerStoragePaths.h"
#include "Minecraft.Server/Common/StringUtils.h"

namespace
{
    struct NativeDedicatedServerWorldLoadStorageEntry
    {
        std::filesystem::path path;
        std::wstring title;
        std::wstring filename;
        bool hasStubMetadata = false;
        bool isCorrupt = false;
        ServerRuntime::NativeDedicatedServerSaveStub saveStub = {};
    };

    struct NativeDedicatedServerWorldLoadStorageContext
    {
        std::wstring configuredTitle;
        std::string configuredSaveId;
        std::vector<NativeDedicatedServerWorldLoadStorageEntry> entries;
        int matchedIndex = -1;
    };

    static void DestroyNativeDedicatedServerWorldLoadStorageContext(
        void *context)
    {
        delete static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
            context);
    }

    static void SetNativeDedicatedServerWorldStorageTitle(
        const std::wstring &worldName,
        void *context)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext != nullptr)
        {
            storageContext->configuredTitle = worldName;
        }
    }

    static void SetNativeDedicatedServerWorldStorageSaveId(
        const std::string &saveId,
        void *context)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext != nullptr)
        {
            storageContext->configuredSaveId = saveId;
        }
    }

    static void ResetNativeDedicatedServerWorldStorageData(void *context)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == nullptr)
        {
            return;
        }

        storageContext->entries.clear();
        storageContext->matchedIndex = -1;
    }

    static bool EnumerateNativeDedicatedServerWorldSaveEntries(
        NativeDedicatedServerWorldLoadStorageContext *context)
    {
        if (context == nullptr)
        {
            return false;
        }

        context->entries.clear();
        context->matchedIndex = -1;
        const std::filesystem::path rootPath =
            ServerRuntime::GetServerGameHddRootPath();
        std::error_code errorCode;
        if (!std::filesystem::exists(rootPath, errorCode))
        {
            return !errorCode;
        }

        for (const std::filesystem::directory_entry &entry :
            std::filesystem::directory_iterator(rootPath, errorCode))
        {
            if (errorCode)
            {
                return false;
            }
            if (!entry.is_regular_file())
            {
                continue;
            }
            if (entry.path().extension() != ".save")
            {
                continue;
            }

            NativeDedicatedServerWorldLoadStorageEntry saveEntry = {};
            saveEntry.path = entry.path();
            saveEntry.title = entry.path().stem().wstring();
            saveEntry.filename = entry.path().stem().wstring();

            std::ifstream input(entry.path(), std::ios::binary);
            if (input)
            {
                std::string fileText(
                    (std::istreambuf_iterator<char>(input)),
                    std::istreambuf_iterator<char>());
                ServerRuntime::NativeDedicatedServerSaveStub stub = {};
                if (ServerRuntime::ParseNativeDedicatedServerSaveStubText(
                        fileText,
                        &stub))
                {
                    saveEntry.hasStubMetadata = true;
                    saveEntry.saveStub = stub;
                    if (!stub.worldName.empty())
                    {
                        saveEntry.title =
                            ServerRuntime::StringUtils::Utf8ToWide(
                                stub.worldName);
                    }
                    if (!stub.levelId.empty())
                    {
                        saveEntry.filename =
                            ServerRuntime::StringUtils::Utf8ToWide(
                                stub.levelId);
                    }
                }
                else
                {
                    saveEntry.isCorrupt = true;
                }
            }
            else
            {
                saveEntry.isCorrupt = true;
            }

            context->entries.push_back(saveEntry);
        }

        std::sort(
            context->entries.begin(),
            context->entries.end(),
            [](const NativeDedicatedServerWorldLoadStorageEntry &lhs,
                const NativeDedicatedServerWorldLoadStorageEntry &rhs)
            {
                return lhs.path.filename().native() <
                    rhs.path.filename().native();
            });
        return true;
    }

    static bool ClearNativeDedicatedServerWorldSaveQuery(void *context)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == nullptr)
        {
            return false;
        }

        storageContext->entries.clear();
        storageContext->matchedIndex = -1;
        return true;
    }

    static bool BeginNativeDedicatedServerWorldSaveQuery(int, void *context)
    {
        return EnumerateNativeDedicatedServerWorldSaveEntries(
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context));
    }

    static bool PollNativeDedicatedServerWorldSaveQuery(
        DWORD,
        ServerRuntime::DedicatedServerWorldLoadTickProc,
        void *context,
        std::vector<ServerRuntime::DedicatedServerWorldLoadCandidate>
            *outCandidates)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == nullptr || outCandidates == nullptr)
        {
            return false;
        }

        outCandidates->clear();
        outCandidates->reserve(storageContext->entries.size());
        for (size_t i = 0; i < storageContext->entries.size(); ++i)
        {
            ServerRuntime::DedicatedServerWorldLoadCandidate candidate = {};
            candidate.title = storageContext->entries[i].title;
            candidate.filename = storageContext->entries[i].filename;
            outCandidates->push_back(candidate);
        }

        return true;
    }

    static bool BeginNativeDedicatedServerWorldSaveLoad(
        int matchedIndex,
        void *context)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == nullptr ||
            matchedIndex < 0 ||
            matchedIndex >= static_cast<int>(storageContext->entries.size()))
        {
            return false;
        }

        storageContext->matchedIndex = matchedIndex;
        ServerRuntime::ResetNativeDedicatedServerLoadedSaveMetadata();
        const NativeDedicatedServerWorldLoadStorageEntry &matchedEntry =
            storageContext->entries[(size_t)matchedIndex];
        ServerRuntime::RecordNativeDedicatedServerLoadedSavePath(
            matchedEntry.path.string());
        if (matchedEntry.hasStubMetadata)
        {
            ServerRuntime::RecordNativeDedicatedServerLoadedSaveMetadata(
                matchedEntry.path.string(),
                matchedEntry.saveStub);
        }
        return true;
    }

    static bool PollNativeDedicatedServerWorldSaveLoad(
        DWORD,
        ServerRuntime::DedicatedServerWorldLoadTickProc,
        void *context,
        bool *outIsCorrupt)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == nullptr ||
            storageContext->matchedIndex < 0 ||
            storageContext->matchedIndex >=
                static_cast<int>(storageContext->entries.size()))
        {
            return false;
        }

        const NativeDedicatedServerWorldLoadStorageEntry &matchedEntry =
            storageContext->entries[(size_t)storageContext->matchedIndex];
        if (outIsCorrupt != nullptr)
        {
            *outIsCorrupt = matchedEntry.isCorrupt;
        }

        return std::filesystem::is_regular_file(matchedEntry.path);
    }

    static bool ReadNativeDedicatedServerWorldSaveBytes(
        void *context,
        std::vector<unsigned char> *outBytes)
    {
        NativeDedicatedServerWorldLoadStorageContext *storageContext =
            static_cast<NativeDedicatedServerWorldLoadStorageContext *>(
                context);
        if (storageContext == nullptr ||
            outBytes == nullptr ||
            storageContext->matchedIndex < 0 ||
            storageContext->matchedIndex >=
                static_cast<int>(storageContext->entries.size()))
        {
            return false;
        }

        const std::filesystem::path savePath =
            storageContext->entries[storageContext->matchedIndex].path;
        std::ifstream input(savePath, std::ios::binary);
        if (!input)
        {
            outBytes->clear();
            return false;
        }

        input.seekg(0, std::ios::end);
        const std::streamoff fileSize = input.tellg();
        if (fileSize <= 0)
        {
            outBytes->clear();
            return false;
        }

        outBytes->assign(
            static_cast<size_t>(fileSize),
            0);
        input.seekg(0, std::ios::beg);
        input.read(
            reinterpret_cast<char *>(&(*outBytes)[0]),
            fileSize);
        return input.good() || input.eof();
    }
}

namespace ServerRuntime
{
    DedicatedServerWorldLoadStorageRuntime
    CreateDedicatedServerPlatformWorldLoadStorageRuntime()
    {
        NativeDedicatedServerWorldLoadStorageContext *context =
            new NativeDedicatedServerWorldLoadStorageContext();
        ResetNativeDedicatedServerLoadedSaveMetadata();
        DedicatedServerWorldLoadStorageRuntime runtime = {};
        runtime.storageHooks.setWorldTitleProc =
            &SetNativeDedicatedServerWorldStorageTitle;
        runtime.storageHooks.setWorldTitleContext = context;
        runtime.storageHooks.setWorldSaveIdProc =
            &SetNativeDedicatedServerWorldStorageSaveId;
        runtime.storageHooks.setWorldSaveIdContext = context;
        runtime.storageHooks.resetSaveDataProc =
            &ResetNativeDedicatedServerWorldStorageData;
        runtime.storageHooks.resetSaveDataContext = context;
        runtime.loadHooks.clearSavesProc =
            &ClearNativeDedicatedServerWorldSaveQuery;
        runtime.loadHooks.beginQueryProc =
            &BeginNativeDedicatedServerWorldSaveQuery;
        runtime.loadHooks.pollQueryProc =
            &PollNativeDedicatedServerWorldSaveQuery;
        runtime.loadHooks.beginLoadProc =
            &BeginNativeDedicatedServerWorldSaveLoad;
        runtime.loadHooks.pollLoadProc =
            &PollNativeDedicatedServerWorldSaveLoad;
        runtime.loadHooks.readBytesProc =
            &ReadNativeDedicatedServerWorldSaveBytes;
        runtime.loadHooks.context = context;
        runtime.platformContext = context;
        runtime.destroyContextProc =
            &DestroyNativeDedicatedServerWorldLoadStorageContext;
        return runtime;
    }
}
