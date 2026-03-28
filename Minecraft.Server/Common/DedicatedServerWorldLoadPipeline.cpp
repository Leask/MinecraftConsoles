#include "DedicatedServerWorldLoadPipeline.h"

#include "DedicatedServerWorldSaveSelection.h"
#include "StringUtils.h"

namespace ServerRuntime
{
    namespace
    {
        bool BuildWorldSaveMatchEntries(
            const std::vector<DedicatedServerWorldLoadCandidate> &candidates,
            std::vector<DedicatedServerWorldSaveMatchEntry> *outEntries)
        {
            if (outEntries == nullptr)
            {
                return false;
            }

            outEntries->clear();
            outEntries->reserve(candidates.size());
            for (size_t i = 0; i < candidates.size(); ++i)
            {
                DedicatedServerWorldSaveMatchEntry matchEntry = {};
                matchEntry.title = candidates[i].title;
                matchEntry.filename = candidates[i].filename;
                outEntries->push_back(matchEntry);
            }
            return true;
        }

        std::string ResolveDedicatedServerLoadedSaveId(
            const DedicatedServerWorldTarget &worldTarget,
            const DedicatedServerWorldLoadCandidate &matchedCandidate)
        {
            if (!matchedCandidate.filename.empty())
            {
                return StringUtils::WideToUtf8(matchedCandidate.filename);
            }

            return worldTarget.saveId;
        }
    }

    EDedicatedServerWorldLoadStatus LoadDedicatedServerWorldData(
        const DedicatedServerWorldTarget &worldTarget,
        int actionPad,
        DedicatedServerWorldLoadTickProc tickProc,
        const DedicatedServerWorldLoadHooks &hooks,
        DedicatedServerWorldLoadPayload *outPayload)
    {
        if (outPayload == nullptr ||
            hooks.clearSavesProc == nullptr ||
            hooks.beginQueryProc == nullptr ||
            hooks.pollQueryProc == nullptr ||
            hooks.beginLoadProc == nullptr ||
            hooks.pollLoadProc == nullptr ||
            hooks.readBytesProc == nullptr)
        {
            return eDedicatedServerWorldLoad_Failed;
        }

        *outPayload = DedicatedServerWorldLoadPayload{};
        if (!hooks.clearSavesProc(hooks.context))
        {
            return eDedicatedServerWorldLoad_Failed;
        }
        if (!hooks.beginQueryProc(actionPad, hooks.context))
        {
            return eDedicatedServerWorldLoad_Failed;
        }

        std::vector<DedicatedServerWorldLoadCandidate> candidates;
        if (!hooks.pollQueryProc(
                10000,
                tickProc,
                hooks.context,
                &candidates))
        {
            return eDedicatedServerWorldLoad_Failed;
        }

        std::vector<DedicatedServerWorldSaveMatchEntry> matchEntries;
        if (!BuildWorldSaveMatchEntries(candidates, &matchEntries))
        {
            return eDedicatedServerWorldLoad_Failed;
        }

        const DedicatedServerWorldSaveSelectionResult selection =
            SelectDedicatedServerWorldSaveEntry(
                matchEntries.empty() ? nullptr : &matchEntries[0],
                static_cast<int>(matchEntries.size()),
                worldTarget.worldName,
                worldTarget.saveId);
        if (selection.matchedIndex < 0 ||
            selection.matchedIndex >= static_cast<int>(candidates.size()))
        {
            return eDedicatedServerWorldLoad_NotFound;
        }

        const DedicatedServerWorldLoadCandidate &matchedCandidate =
            candidates[selection.matchedIndex];
        outPayload->matchedTitle = matchedCandidate.title.empty()
            ? worldTarget.worldName
            : matchedCandidate.title;
        outPayload->matchedFilename = matchedCandidate.filename;
        outPayload->resolvedSaveId = ResolveDedicatedServerLoadedSaveId(
            worldTarget,
            matchedCandidate);

        if (!hooks.beginLoadProc(selection.matchedIndex, hooks.context))
        {
            return eDedicatedServerWorldLoad_Failed;
        }

        bool isCorrupt = false;
        if (!hooks.pollLoadProc(
                15000,
                tickProc,
                hooks.context,
                &isCorrupt) ||
            isCorrupt)
        {
            return eDedicatedServerWorldLoad_Failed;
        }

        if (!hooks.readBytesProc(hooks.context, &outPayload->bytes) ||
            outPayload->bytes.empty())
        {
            return eDedicatedServerWorldLoad_Failed;
        }

        return eDedicatedServerWorldLoad_Loaded;
    }
}
