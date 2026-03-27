#include "DedicatedServerWorldSaveSelection.h"

#include <cwctype>

#include "StringUtils.h"

namespace
{
    bool EqualsWideIgnoreCase(
        const std::wstring &lhs,
        const std::wstring &rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (size_t i = 0; i < lhs.size(); ++i)
        {
            if (std::towlower(lhs[i]) != std::towlower(rhs[i]))
            {
                return false;
            }
        }

        return true;
    }
}

namespace ServerRuntime
{
    bool MatchesDedicatedServerWorldName(
        const DedicatedServerWorldSaveMatchEntry &entry,
        const std::wstring &targetWorldName)
    {
        if (targetWorldName.empty())
        {
            return false;
        }

        return (!entry.title.empty() &&
                EqualsWideIgnoreCase(entry.title, targetWorldName)) ||
            (!entry.filename.empty() &&
                EqualsWideIgnoreCase(entry.filename, targetWorldName));
    }

    bool MatchesDedicatedServerSaveFilename(
        const DedicatedServerWorldSaveMatchEntry &entry,
        const std::string &targetSaveFilename)
    {
        if (targetSaveFilename.empty())
        {
            return false;
        }

        return StringUtils::ToLowerAscii(
                StringUtils::WideToUtf8(entry.filename)) ==
            StringUtils::ToLowerAscii(targetSaveFilename);
    }

    DedicatedServerWorldSaveSelectionResult
    SelectDedicatedServerWorldSaveEntry(
        const DedicatedServerWorldSaveMatchEntry *entries,
        int entryCount,
        const std::wstring &targetWorldName,
        const std::string &targetSaveFilename)
    {
        DedicatedServerWorldSaveSelectionResult result = {};
        if (entries == nullptr || entryCount <= 0)
        {
            return result;
        }

        if (!targetSaveFilename.empty())
        {
            for (int i = 0; i < entryCount; ++i)
            {
                if (MatchesDedicatedServerSaveFilename(
                        entries[i],
                        targetSaveFilename))
                {
                    result.matchedIndex = i;
                    result.matchedBySaveId = true;
                    return result;
                }
            }
        }

        for (int i = 0; i < entryCount; ++i)
        {
            if (MatchesDedicatedServerWorldName(
                    entries[i],
                    targetWorldName))
            {
                result.matchedIndex = i;
                return result;
            }
        }

        return result;
    }
}
