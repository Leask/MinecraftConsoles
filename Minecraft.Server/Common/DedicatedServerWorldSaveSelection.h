#pragma once

#include <string>

namespace ServerRuntime
{
    struct DedicatedServerWorldSaveMatchEntry
    {
        std::wstring title;
        std::wstring filename;
    };

    struct DedicatedServerWorldSaveSelectionResult
    {
        int matchedIndex = -1;
        bool matchedBySaveId = false;
    };

    bool MatchesDedicatedServerWorldName(
        const DedicatedServerWorldSaveMatchEntry &entry,
        const std::wstring &targetWorldName);

    bool MatchesDedicatedServerSaveFilename(
        const DedicatedServerWorldSaveMatchEntry &entry,
        const std::string &targetSaveFilename);

    DedicatedServerWorldSaveSelectionResult
    SelectDedicatedServerWorldSaveEntry(
        const DedicatedServerWorldSaveMatchEntry *entries,
        int entryCount,
        const std::wstring &targetWorldName,
        const std::string &targetSaveFilename);
}
