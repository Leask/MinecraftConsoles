#include "NativeDedicatedServerHostedGameSession.h"

#include <mutex>
#include <string>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerLoadedSaveState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "Minecraft.Server/Common/StringUtils.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameWorker.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        static constexpr std::uint64_t kNativeHostedSessionHashOffset =
            14695981039346656037ULL;
        static constexpr std::uint64_t kNativeHostedSessionHashPrime =
            1099511628211ULL;

        struct NativeDedicatedServerHostedGameSessionState
        {
            NativeDedicatedServerHostedGameSessionSnapshot snapshot = {};
            std::uint64_t baseSaveGeneration = 0;
            std::uint64_t baseStateChecksum = 0;
            std::uint64_t baseWorkerTickCount = 0;
            std::uint64_t baseCompletedWorkerActions = 0;
            std::uint64_t baseProcessedAutosaveCommands = 0;
            std::uint64_t baseProcessedSaveCommands = 0;
            std::uint64_t baseProcessedStopCommands = 0;
            std::uint64_t baseProcessedHaltCommands = 0;
            std::uint64_t baseGameplayLoopIterations = 0;
        };

        std::mutex g_nativeHostedSessionMutex;
        NativeDedicatedServerHostedGameSessionState
            g_nativeHostedSessionState = {};

        std::uint64_t ComputeNativeHostedSessionBytesChecksum(
            const void *data,
            std::int64_t fileSize)
        {
            if (data == nullptr || fileSize <= 0)
            {
                return 0;
            }

            const unsigned char *bytes =
                static_cast<const unsigned char *>(data);
            std::uint64_t checksum = kNativeHostedSessionHashOffset;
            for (std::int64_t i = 0; i < fileSize; ++i)
            {
                checksum ^= bytes[i];
                checksum *= kNativeHostedSessionHashPrime;
            }

            return checksum;
        }

        std::uint64_t MixNativeHostedSessionHash(
            std::uint64_t checksum,
            std::uint64_t value)
        {
            for (int i = 0; i < 8; ++i)
            {
                checksum ^= (value & 0xffU);
                checksum *= kNativeHostedSessionHashPrime;
                value >>= 8;
            }

            return checksum;
        }

        std::uint64_t MixNativeHostedSessionStringHash(
            std::uint64_t checksum,
            const std::string &value)
        {
            for (size_t i = 0; i < value.size(); ++i)
            {
                checksum ^= static_cast<unsigned char>(value[i]);
                checksum *= kNativeHostedSessionHashPrime;
            }

            return MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(value.size()));
        }

        void RefreshNativeHostedSessionStateChecksum(
            NativeDedicatedServerHostedGameSessionState *state)
        {
            if (state == nullptr)
            {
                return;
            }

            state->snapshot.saveGeneration =
                state->baseSaveGeneration +
                state->snapshot.observedAutosaveCompletions;
            std::uint64_t checksum =
                state->baseStateChecksum != 0
                    ? state->baseStateChecksum
                    : kNativeHostedSessionHashOffset;
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.worldName);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.worldSaveId);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.savePath);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.storageRoot);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.hostName);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.bindIp);
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.startupResult));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.runtimePhase));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.localUsersMask));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.configuredPort));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.listenerPort));
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.publicSlots);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.privateSlots);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.payloadChecksum);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.sessionTicks);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.observedAutosaveCompletions);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerPendingWorldActionTicks);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerPendingAutosaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerPendingSaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerPendingStopCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerPendingHaltCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerTickCount);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.completedWorkerActions);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.processedAutosaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.processedSaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.processedStopCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.processedHaltCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.lastQueuedCommandId);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.activeCommandId);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.activeCommandTicksRemaining);
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(
                    state->snapshot.activeCommandKind));
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.lastProcessedCommandId);
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(
                    state->snapshot.lastProcessedCommandKind));
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.acceptedConnections);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.remoteCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.autosaveRequests);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.gameplayLoopIterations);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.platformTickCount);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.saveGeneration);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.sessionStartMs);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.lastPersistedFileTime);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.lastPersistedAutosaveCompletions);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.lastPersistedSavePath);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.startupThreadIterations);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.startupThreadDurationMs);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.hostedThreadTicks);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.stoppedMs);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.loadedFromSave ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.payloadValidated ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.threadInvoked ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.onlineGame ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.privateGame ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.fakeLocalPlayerJoined ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.active ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worldActionIdle ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.appShutdownRequested ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.gameplayHalted ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.stopSignalValid ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.hostedThreadActive ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.initialSaveRequested ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.initialSaveCompleted ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.initialSaveTimedOut ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.sessionCompleted ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.requestedAppShutdown ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.shutdownHaltedGameplay ? 1U : 0U);
            state->snapshot.stateChecksum = checksum;
        }

        void ApplyNativeHostedSessionWorkerSnapshot(
            NativeDedicatedServerHostedGameSessionState *state,
            const NativeDedicatedServerHostedGameWorkerSnapshot &workerSnapshot)
        {
            if (state == nullptr)
            {
                return;
            }

            state->snapshot.workerPendingWorldActionTicks =
                workerSnapshot.pendingWorldActionTicks;
            state->snapshot.workerPendingAutosaveCommands =
                workerSnapshot.pendingAutosaveCommands;
            state->snapshot.workerPendingSaveCommands =
                workerSnapshot.pendingSaveCommands;
            state->snapshot.workerPendingStopCommands =
                workerSnapshot.pendingStopCommands;
            state->snapshot.workerPendingHaltCommands =
                workerSnapshot.pendingHaltCommands;
            state->snapshot.workerTickCount =
                state->baseWorkerTickCount +
                workerSnapshot.workerTickCount;
            state->snapshot.completedWorkerActions =
                state->baseCompletedWorkerActions +
                workerSnapshot.completedWorldActions;
            state->snapshot.processedAutosaveCommands =
                state->baseProcessedAutosaveCommands +
                workerSnapshot.processedAutosaveCommands;
            state->snapshot.processedSaveCommands =
                state->baseProcessedSaveCommands +
                workerSnapshot.processedSaveCommands;
            state->snapshot.processedStopCommands =
                state->baseProcessedStopCommands +
                workerSnapshot.processedStopCommands;
            state->snapshot.processedHaltCommands =
                state->baseProcessedHaltCommands +
                workerSnapshot.processedHaltCommands;
            state->snapshot.lastQueuedCommandId =
                workerSnapshot.lastQueuedCommandId;
            state->snapshot.activeCommandId =
                workerSnapshot.activeCommandId;
            state->snapshot.activeCommandTicksRemaining =
                workerSnapshot.activeCommandTicksRemaining;
            state->snapshot.activeCommandKind =
                workerSnapshot.activeCommandKind;
            state->snapshot.lastProcessedCommandId =
                workerSnapshot.lastProcessedCommandId;
            state->snapshot.lastProcessedCommandKind =
                workerSnapshot.lastProcessedCommandKind;
            state->snapshot.worldActionIdle =
                workerSnapshot.pendingWorldActionTicks == 0 &&
                workerSnapshot.pendingAutosaveCommands == 0 &&
                workerSnapshot.pendingSaveCommands == 0 &&
                workerSnapshot.pendingStopCommands == 0 &&
                workerSnapshot.pendingHaltCommands == 0 &&
                workerSnapshot.activeCommandKind ==
                    eNativeDedicatedServerHostedGameWorkerCommand_None;
        }

        void RefreshNativeHostedSessionPhase(
            NativeDedicatedServerHostedGameSessionState *state)
        {
            if (state == nullptr)
            {
                return;
            }

            if (!state->snapshot.active &&
                state->snapshot.runtimePhase ==
                    eDedicatedServerHostedGameRuntimePhase_Stopped)
            {
                return;
            }

            if (state->snapshot.startupResult != 0)
            {
                state->snapshot.runtimePhase =
                    eDedicatedServerHostedGameRuntimePhase_Failed;
                return;
            }

            if (state->snapshot.appShutdownRequested ||
                state->snapshot.gameplayHalted)
            {
                state->snapshot.runtimePhase =
                    eDedicatedServerHostedGameRuntimePhase_ShutdownRequested;
                return;
            }

            if (state->snapshot.hostedThreadActive ||
                state->snapshot.threadInvoked)
            {
                state->snapshot.runtimePhase =
                    eDedicatedServerHostedGameRuntimePhase_Running;
                return;
            }

            if (state->snapshot.active ||
                state->snapshot.payloadValidated ||
                state->snapshot.startupResult == 0)
            {
                state->snapshot.runtimePhase =
                    eDedicatedServerHostedGameRuntimePhase_Startup;
            }
        }

        void RefreshNativeHostedSessionActive(
            NativeDedicatedServerHostedGameSessionState *state)
        {
            if (state == nullptr)
            {
                return;
            }

            if (state->snapshot.runtimePhase ==
                eDedicatedServerHostedGameRuntimePhase_Stopped)
            {
                state->snapshot.active = false;
                return;
            }

            if (state->snapshot.startupResult != 0)
            {
                state->snapshot.active = false;
                return;
            }

            if (state->snapshot.hostedThreadActive ||
                state->snapshot.threadInvoked ||
                state->snapshot.payloadValidated ||
                state->snapshot.startupResult == 0)
            {
                state->snapshot.active = true;
            }
        }

        NativeDedicatedServerHostedGameSessionSnapshot
        BuildNativeHostedSessionStartupProjectionSnapshot(
            const NativeDedicatedServerHostedGameSessionSnapshot &snapshot,
            int startupResult,
            bool threadInvoked)
        {
            NativeDedicatedServerHostedGameSessionSnapshot
                projectedSnapshot = snapshot;
            projectedSnapshot.startupResult = startupResult;
            projectedSnapshot.threadInvoked = threadInvoked;

            if (projectedSnapshot.runtimePhase ==
                eDedicatedServerHostedGameRuntimePhase_Stopped)
            {
                projectedSnapshot.active = false;
                return projectedSnapshot;
            }

            if (startupResult != 0)
            {
                projectedSnapshot.active = false;
                projectedSnapshot.runtimePhase =
                    eDedicatedServerHostedGameRuntimePhase_Failed;
                return projectedSnapshot;
            }

            projectedSnapshot.active =
                projectedSnapshot.hostedThreadActive ||
                projectedSnapshot.threadInvoked ||
                projectedSnapshot.payloadValidated ||
                startupResult == 0;
            if (projectedSnapshot.appShutdownRequested ||
                projectedSnapshot.gameplayHalted)
            {
                projectedSnapshot.runtimePhase =
                    eDedicatedServerHostedGameRuntimePhase_ShutdownRequested;
                return projectedSnapshot;
            }

            if (projectedSnapshot.hostedThreadActive ||
                projectedSnapshot.threadInvoked)
            {
                projectedSnapshot.runtimePhase =
                    eDedicatedServerHostedGameRuntimePhase_Running;
                return projectedSnapshot;
            }

            projectedSnapshot.runtimePhase =
                eDedicatedServerHostedGameRuntimePhase_Startup;
            return projectedSnapshot;
        }
    }

    void ResetNativeDedicatedServerHostedGameSessionCoreState()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState = NativeDedicatedServerHostedGameSessionState{};
    }

    void ResetNativeDedicatedServerHostedGameSessionState()
    {
        ResetNativeDedicatedServerHostedGameHostState();
        ResetNativeDedicatedServerHostedGameWorkerState();
        ResetNativeDedicatedServerHostedGameSessionCoreState();
        ResetDedicatedServerHostedGameRuntimeSnapshot();
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot();
    }

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData)
    {
        bool startupPayloadValidated = initData.saveData == nullptr;
        NativeDedicatedServerSaveStub saveStub = {};
        bool hasParsedSaveStub = false;
        if (initData.saveData != nullptr &&
            initData.saveData->data != nullptr &&
            initData.saveData->fileSize > 0)
        {
            const char *payloadBytes =
                static_cast<const char *>(initData.saveData->data);
            const std::string payloadText(
                payloadBytes,
                payloadBytes + initData.saveData->fileSize);
            hasParsedSaveStub = ParseNativeDedicatedServerSaveStubText(
                payloadText,
                &saveStub);
            startupPayloadValidated = hasParsedSaveStub;
        }

        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState = NativeDedicatedServerHostedGameSessionState{};
        g_nativeHostedSessionState.snapshot.startAttempted = true;
        g_nativeHostedSessionState.snapshot.active = startupPayloadValidated;
        g_nativeHostedSessionState.snapshot.loadedFromSave =
            initData.saveData != nullptr;
        g_nativeHostedSessionState.snapshot.payloadValidated =
            startupPayloadValidated;
        g_nativeHostedSessionState.snapshot.resolvedSeed = initData.seed;
        g_nativeHostedSessionState.snapshot.hostSettings =
            initData.settings;
        g_nativeHostedSessionState.snapshot.localUsersMask =
            initData.localUsersMask;
        g_nativeHostedSessionState.snapshot.onlineGame =
            initData.onlineGame;
        g_nativeHostedSessionState.snapshot.privateGame =
            initData.privateGame;
        g_nativeHostedSessionState.snapshot.publicSlots =
            initData.publicSlots;
        g_nativeHostedSessionState.snapshot.privateSlots =
            initData.privateSlots;
        g_nativeHostedSessionState.snapshot.fakeLocalPlayerJoined =
            initData.fakeLocalPlayerJoined;
        g_nativeHostedSessionState.snapshot.dedicatedNoLocalHostPlayer =
            initData.dedicatedNoLocalHostPlayer;
        g_nativeHostedSessionState.snapshot.worldSizeChunks =
            initData.xzSize;
        g_nativeHostedSessionState.snapshot.worldHellScale =
            initData.hellScale;
        g_nativeHostedSessionState.snapshot.runtimePhase =
            startupPayloadValidated
                ? eDedicatedServerHostedGameRuntimePhase_Startup
                : eDedicatedServerHostedGameRuntimePhase_Failed;
        g_nativeHostedSessionState.snapshot.payloadChecksum =
            ComputeNativeHostedSessionBytesChecksum(
                initData.saveData != nullptr ? initData.saveData->data : nullptr,
                initData.saveData != nullptr ? initData.saveData->fileSize : 0);

        if (initData.saveData != nullptr &&
            initData.saveData->data != nullptr &&
            initData.saveData->fileSize > 0)
        {
            g_nativeHostedSessionState.snapshot.savePayloadBytes =
                initData.saveData->fileSize;
            g_nativeHostedSessionState.snapshot.savePayloadName =
                StringUtils::WideToUtf8(
                    initData.saveData->saveName);
            if (hasParsedSaveStub)
            {
                g_nativeHostedSessionState.baseSaveGeneration =
                    saveStub.saveGeneration;
                g_nativeHostedSessionState.baseStateChecksum =
                    saveStub.stateChecksum;
                g_nativeHostedSessionState.baseWorkerTickCount =
                    saveStub.workerTickCount;
                g_nativeHostedSessionState.baseCompletedWorkerActions =
                    saveStub.completedWorkerActions;
                g_nativeHostedSessionState.baseProcessedAutosaveCommands =
                    saveStub.processedAutosaveCommands;
                g_nativeHostedSessionState.baseProcessedSaveCommands =
                    saveStub.processedSaveCommands;
                g_nativeHostedSessionState.baseProcessedStopCommands =
                    saveStub.processedStopCommands;
                g_nativeHostedSessionState.baseProcessedHaltCommands =
                    saveStub.processedHaltCommands;
                g_nativeHostedSessionState.baseGameplayLoopIterations =
                    saveStub.gameplayLoopIterations;
                g_nativeHostedSessionState.snapshot.sessionTicks =
                    saveStub.hostedThreadTicks;
                g_nativeHostedSessionState.snapshot.workerTickCount =
                    saveStub.workerTickCount;
                g_nativeHostedSessionState.snapshot.completedWorkerActions =
                    saveStub.completedWorkerActions;
                g_nativeHostedSessionState.snapshot
                    .workerPendingAutosaveCommands =
                        saveStub.workerPendingAutosaveCommands;
                g_nativeHostedSessionState.snapshot
                    .processedAutosaveCommands =
                        saveStub.processedAutosaveCommands;
                g_nativeHostedSessionState.snapshot.processedSaveCommands =
                    saveStub.processedSaveCommands;
                g_nativeHostedSessionState.snapshot.processedStopCommands =
                    saveStub.processedStopCommands;
                g_nativeHostedSessionState.snapshot.workerPendingHaltCommands =
                    saveStub.workerPendingHaltCommands;
                g_nativeHostedSessionState.snapshot.processedHaltCommands =
                    saveStub.processedHaltCommands;
                g_nativeHostedSessionState.snapshot.lastQueuedCommandId =
                    saveStub.lastQueuedCommandId;
                g_nativeHostedSessionState.snapshot.activeCommandId =
                    saveStub.activeCommandId;
                g_nativeHostedSessionState.snapshot
                    .activeCommandTicksRemaining =
                        saveStub.activeCommandTicksRemaining;
                g_nativeHostedSessionState.snapshot.activeCommandKind =
                    (ENativeDedicatedServerHostedGameWorkerCommandKind)
                        saveStub.activeCommandKind;
                g_nativeHostedSessionState.snapshot.lastProcessedCommandId =
                    saveStub.lastProcessedCommandId;
                g_nativeHostedSessionState.snapshot.lastProcessedCommandKind =
                    (ENativeDedicatedServerHostedGameWorkerCommandKind)
                        saveStub.lastProcessedCommandKind;
                g_nativeHostedSessionState.snapshot.gameplayLoopIterations =
                    saveStub.gameplayLoopIterations;
                g_nativeHostedSessionState.snapshot.lastPersistedFileTime =
                    saveStub.savedAtFileTime;
                g_nativeHostedSessionState.snapshot
                    .lastPersistedAutosaveCompletions =
                    saveStub.autosaveCompletions;
            }
        }

        const NativeDedicatedServerLoadedSaveMetadata loadedSaveMetadata =
            GetNativeDedicatedServerLoadedSaveMetadata();
        if (loadedSaveMetadata.available)
        {
            g_nativeHostedSessionState.snapshot.loadedSavePath =
                loadedSaveMetadata.savePath;
        }
        if (loadedSaveMetadata.hasSaveStub)
        {
            g_nativeHostedSessionState.snapshot
                .loadedSaveMetadataAvailable = true;
            g_nativeHostedSessionState.snapshot.previousStartupMode =
                loadedSaveMetadata.saveStub.startupMode;
            g_nativeHostedSessionState.snapshot.previousSessionPhase =
                loadedSaveMetadata.saveStub.sessionPhase;
            g_nativeHostedSessionState.snapshot.previousRemoteCommands =
                loadedSaveMetadata.saveStub.remoteCommands;
            g_nativeHostedSessionState.snapshot
                .previousAutosaveCompletions =
                    loadedSaveMetadata.saveStub.autosaveCompletions;
            g_nativeHostedSessionState.snapshot
                .previousWorkerPendingWorldActionTicks =
                    loadedSaveMetadata.saveStub
                        .workerPendingWorldActionTicks;
            g_nativeHostedSessionState.snapshot
                .previousWorkerPendingAutosaveCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingAutosaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorkerPendingSaveCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingSaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorkerPendingStopCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingStopCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorkerPendingHaltCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingHaltCommands;
            g_nativeHostedSessionState.snapshot.previousWorkerTickCount =
                loadedSaveMetadata.saveStub.workerTickCount;
            g_nativeHostedSessionState.snapshot
                .previousCompletedWorkerActions =
                    loadedSaveMetadata.saveStub.completedWorkerActions;
            g_nativeHostedSessionState.snapshot
                .previousProcessedAutosaveCommands =
                    loadedSaveMetadata.saveStub
                        .processedAutosaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousProcessedSaveCommands =
                    loadedSaveMetadata.saveStub.processedSaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousProcessedStopCommands =
                    loadedSaveMetadata.saveStub.processedStopCommands;
            g_nativeHostedSessionState.snapshot
                .previousProcessedHaltCommands =
                    loadedSaveMetadata.saveStub.processedHaltCommands;
            g_nativeHostedSessionState.snapshot
                .previousLastQueuedCommandId =
                    loadedSaveMetadata.saveStub.lastQueuedCommandId;
            g_nativeHostedSessionState.snapshot.previousActiveCommandId =
                loadedSaveMetadata.saveStub.activeCommandId;
            g_nativeHostedSessionState.snapshot
                .previousActiveCommandTicksRemaining =
                    loadedSaveMetadata.saveStub
                        .activeCommandTicksRemaining;
            g_nativeHostedSessionState.snapshot.previousActiveCommandKind =
                (ENativeDedicatedServerHostedGameWorkerCommandKind)
                    loadedSaveMetadata.saveStub.activeCommandKind;
            g_nativeHostedSessionState.snapshot
                .previousLastProcessedCommandId =
                    loadedSaveMetadata.saveStub.lastProcessedCommandId;
            g_nativeHostedSessionState.snapshot
                .previousLastProcessedCommandKind =
                    (ENativeDedicatedServerHostedGameWorkerCommandKind)
                        loadedSaveMetadata.saveStub
                            .lastProcessedCommandKind;
            g_nativeHostedSessionState.snapshot
                .previousPlatformTickCount =
                    loadedSaveMetadata.saveStub.platformTickCount;
            g_nativeHostedSessionState.snapshot.previousUptimeMs =
                loadedSaveMetadata.saveStub.uptimeMs;
            g_nativeHostedSessionState.snapshot
                .previousGameplayLoopIterations =
                    loadedSaveMetadata.saveStub
                        .gameplayLoopIterations;
            g_nativeHostedSessionState.snapshot.previousHostSettings =
                loadedSaveMetadata.saveStub.hostSettings;
            g_nativeHostedSessionState.snapshot
                .previousDedicatedNoLocalHostPlayer =
                    loadedSaveMetadata.saveStub
                        .dedicatedNoLocalHostPlayer;
            g_nativeHostedSessionState.snapshot
                .previousWorldSizeChunks =
                    loadedSaveMetadata.saveStub.worldSizeChunks;
            g_nativeHostedSessionState.snapshot.previousWorldHellScale =
                (unsigned char)
                    loadedSaveMetadata.saveStub.worldHellScale;
            g_nativeHostedSessionState.snapshot.previousOnlineGame =
                loadedSaveMetadata.saveStub.onlineGame;
            g_nativeHostedSessionState.snapshot.previousPrivateGame =
                loadedSaveMetadata.saveStub.privateGame;
            g_nativeHostedSessionState.snapshot
                .previousFakeLocalPlayerJoined =
                    loadedSaveMetadata.saveStub.fakeLocalPlayerJoined;
            g_nativeHostedSessionState.snapshot.previousPublicSlots =
                (unsigned char)loadedSaveMetadata.saveStub.publicSlots;
            g_nativeHostedSessionState.snapshot.previousPrivateSlots =
                (unsigned char)loadedSaveMetadata.saveStub.privateSlots;
            g_nativeHostedSessionState.snapshot
                .previousSavePayloadChecksum =
                    loadedSaveMetadata.saveStub.payloadChecksum;
            g_nativeHostedSessionState.snapshot.previousSaveGeneration =
                loadedSaveMetadata.saveStub.saveGeneration;
            g_nativeHostedSessionState.snapshot
                .previousSessionStateChecksum =
                    loadedSaveMetadata.saveStub.stateChecksum;
            g_nativeHostedSessionState.snapshot
                .previousStartupPayloadPresent =
                    loadedSaveMetadata.saveStub
                        .startupPayloadPresent;
            g_nativeHostedSessionState.snapshot
                .previousStartupPayloadValidated =
                    loadedSaveMetadata.saveStub
                        .startupPayloadValidated;
            g_nativeHostedSessionState.snapshot
                .previousStartupThreadIterations =
                    loadedSaveMetadata.saveStub
                        .startupThreadIterations;
            g_nativeHostedSessionState.snapshot
                .previousStartupThreadDurationMs =
                    loadedSaveMetadata.saveStub
                        .startupThreadDurationMs;
            g_nativeHostedSessionState.snapshot
                .previousHostedThreadActive =
                    loadedSaveMetadata.saveStub.hostedThreadActive;
            g_nativeHostedSessionState.snapshot.previousHostedThreadTicks =
                loadedSaveMetadata.saveStub.hostedThreadTicks;
            g_nativeHostedSessionState.snapshot
                .previousSessionCompleted =
                    loadedSaveMetadata.saveStub.sessionCompleted;
            g_nativeHostedSessionState.snapshot
                .previousRequestedAppShutdown =
                    loadedSaveMetadata.saveStub.requestedAppShutdown;
            g_nativeHostedSessionState.snapshot
                .previousShutdownHaltedGameplay =
                    loadedSaveMetadata.saveStub
                        .shutdownHaltedGameplay;
        }

        RefreshNativeHostedSessionActive(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
        return startupPayloadValidated;
    }

    bool StartNativeDedicatedServerHostedGameSessionAndProjectStartup(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs)
    {
        const bool startupPayloadValidated =
            StartNativeDedicatedServerHostedGameSession(initData);
        ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
            startupThreadIterations,
            startupThreadDurationMs,
            nowMs);
        return startupPayloadValidated;
    }

    void ObserveNativeDedicatedServerHostedGameSessionStartupResult(
        int startupResult,
        bool threadInvoked)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.startupResult = startupResult;
        g_nativeHostedSessionState.snapshot.threadInvoked = threadInvoked;
        RefreshNativeHostedSessionActive(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionStartupResult(
            startupResult,
            threadInvoked);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
        const NativeDedicatedServerHostedGameSessionSnapshot &snapshot,
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionStartupResult(
            startupResult,
            threadInvoked);
        ProjectNativeDedicatedServerHostedGameSessionSnapshotToRuntimeSnapshot(
            BuildNativeHostedSessionStartupProjectionSnapshot(
                snapshot,
                startupResult,
                threadInvoked),
            nowMs);
    }

    int FinalizeNativeDedicatedServerHostedGameSessionStartupAndProject(
        int startupResult,
        bool threadInvoked,
        const NativeDedicatedServerHostedGameSessionSnapshot *snapshot,
        std::uint64_t nowMs)
    {
        if (snapshot != nullptr)
        {
            ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                *snapshot,
                startupResult,
                threadInvoked,
                nowMs);
        }
        else
        {
            ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                startupResult,
                threadInvoked,
                nowMs);
        }

        return startupResult;
    }

    void TickNativeDedicatedServerHostedGameSession(
        bool hostedThreadActive)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        if (!g_nativeHostedSessionState.snapshot.active)
        {
            return;
        }

        ++g_nativeHostedSessionState.snapshot.sessionTicks;
        ++g_nativeHostedSessionState.snapshot.gameplayLoopIterations;
        g_nativeHostedSessionState.snapshot.hostedThreadActive =
            hostedThreadActive;
        if (hostedThreadActive)
        {
            g_nativeHostedSessionState.snapshot.hostedThreadTicks =
                g_nativeHostedSessionState.snapshot.sessionTicks;
        }
        RefreshNativeHostedSessionActive(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void TickNativeDedicatedServerHostedGameSessionFrame(
        const NativeDedicatedServerHostedGameSessionFrameInput &frameInput)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        if (!g_nativeHostedSessionState.snapshot.active)
        {
            return;
        }

        ApplyNativeHostedSessionWorkerSnapshot(
            &g_nativeHostedSessionState,
            frameInput.workerSnapshot);
        g_nativeHostedSessionState.snapshot.observedAutosaveCompletions =
            frameInput.autosaveCompletions;
        ++g_nativeHostedSessionState.snapshot.sessionTicks;
        ++g_nativeHostedSessionState.snapshot.gameplayLoopIterations;
        g_nativeHostedSessionState.snapshot.hostedThreadActive =
            frameInput.hostedThreadActive;
        if (frameInput.hostedThreadActive)
        {
            g_nativeHostedSessionState.snapshot.hostedThreadTicks =
                g_nativeHostedSessionState.snapshot.sessionTicks;
        }
        RefreshNativeHostedSessionActive(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void TickNativeDedicatedServerHostedGameSessionFrameAndProject(
        const NativeDedicatedServerHostedGameSessionFrameInput &frameInput,
        std::uint64_t nowMs)
    {
        TickNativeDedicatedServerHostedGameSessionFrame(frameInput);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionAutosaves(
        std::uint64_t autosaveCompletions)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.observedAutosaveCompletions =
            autosaveCompletions;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionAutosavesAndProject(
        std::uint64_t autosaveCompletions,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionAutosaves(
            autosaveCompletions);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionActivity(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.acceptedConnections =
            acceptedConnections;
        g_nativeHostedSessionState.snapshot.remoteCommands =
            remoteCommands;
        g_nativeHostedSessionState.snapshot.worldActionIdle =
            worldActionIdle;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionActivityAndProject(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionActivity(
            acceptedConnections,
            remoteCommands,
            worldActionIdle);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionContext(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &savePath,
        const std::string &storageRoot,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
        std::uint64_t sessionStartMs)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.worldName = worldName;
        g_nativeHostedSessionState.snapshot.worldSaveId = worldSaveId;
        g_nativeHostedSessionState.snapshot.savePath = savePath;
        g_nativeHostedSessionState.snapshot.storageRoot = storageRoot;
        g_nativeHostedSessionState.snapshot.hostName = hostName;
        g_nativeHostedSessionState.snapshot.bindIp = bindIp;
        g_nativeHostedSessionState.snapshot.configuredPort = configuredPort;
        g_nativeHostedSessionState.snapshot.listenerPort = listenerPort;
        if (sessionStartMs != 0)
        {
            g_nativeHostedSessionState.snapshot.sessionStartMs =
                sessionStartMs;
            g_nativeHostedSessionState.snapshot.stoppedMs = 0;
        }
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionContextAndProject(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &savePath,
        const std::string &storageRoot,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
        std::uint64_t sessionStartMs,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionContext(
            worldName,
            worldSaveId,
            savePath,
            storageRoot,
            hostName,
            bindIp,
            configuredPort,
            listenerPort,
            sessionStartMs);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionActivation(
        int localUsersMask,
        bool onlineGame,
        bool privateGame,
        unsigned int publicSlots,
        unsigned int privateSlots,
        bool fakeLocalPlayerJoined)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.localUsersMask =
            localUsersMask;
        g_nativeHostedSessionState.snapshot.onlineGame = onlineGame;
        g_nativeHostedSessionState.snapshot.privateGame = privateGame;
        g_nativeHostedSessionState.snapshot.publicSlots = publicSlots;
        g_nativeHostedSessionState.snapshot.privateSlots = privateSlots;
        g_nativeHostedSessionState.snapshot.fakeLocalPlayerJoined =
            fakeLocalPlayerJoined;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionPlatformState(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.autosaveRequests =
            autosaveRequests;
        g_nativeHostedSessionState.snapshot.platformTickCount =
            platformTickCount;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionPlatformStateAndProject(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionPlatformState(
            autosaveRequests,
            platformTickCount);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionPlatformRuntimeStateAndProject(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount,
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionPlatformState(
            autosaveRequests,
            platformTickCount);
        ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
            gameplayLoopIterations,
            appShutdownRequested,
            gameplayHalted,
            stopSignalValid);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        if (gameplayLoopIterations >=
            g_nativeHostedSessionState.snapshot.gameplayLoopIterations)
        {
            g_nativeHostedSessionState.snapshot.gameplayLoopIterations =
                g_nativeHostedSessionState.baseGameplayLoopIterations +
                gameplayLoopIterations;
        }
        g_nativeHostedSessionState.snapshot.appShutdownRequested =
            appShutdownRequested;
        g_nativeHostedSessionState.snapshot.gameplayHalted =
            gameplayHalted;
        g_nativeHostedSessionState.snapshot.stopSignalValid =
            stopSignalValid;
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeStateAndProject(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
            gameplayLoopIterations,
            appShutdownRequested,
            gameplayHalted,
            stopSignalValid);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        if (!savePath.empty())
        {
            g_nativeHostedSessionState.snapshot.lastPersistedSavePath =
                savePath;
        }
        g_nativeHostedSessionState.snapshot.lastPersistedFileTime =
            savedAtFileTime;
        g_nativeHostedSessionState.snapshot
            .lastPersistedAutosaveCompletions = autosaveCompletions;
        if (autosaveCompletions >
            g_nativeHostedSessionState.snapshot.observedAutosaveCompletions)
        {
            g_nativeHostedSessionState.snapshot.observedAutosaveCompletions =
                autosaveCompletions;
        }
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSaveAndProject(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
            savePath,
            savedAtFileTime,
            autosaveCompletions);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionSummary(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.initialSaveRequested =
            initialSaveRequested;
        g_nativeHostedSessionState.snapshot.initialSaveCompleted =
            initialSaveCompleted;
        g_nativeHostedSessionState.snapshot.initialSaveTimedOut =
            initialSaveTimedOut;
        g_nativeHostedSessionState.snapshot.sessionCompleted =
            sessionCompleted;
        g_nativeHostedSessionState.snapshot.requestedAppShutdown =
            requestedAppShutdown;
        g_nativeHostedSessionState.snapshot.shutdownHaltedGameplay =
            shutdownHaltedGameplay;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void FinalizeNativeDedicatedServerHostedGameSession(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay,
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t stoppedMs)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.initialSaveRequested =
            initialSaveRequested;
        g_nativeHostedSessionState.snapshot.initialSaveCompleted =
            initialSaveCompleted;
        g_nativeHostedSessionState.snapshot.initialSaveTimedOut =
            initialSaveTimedOut;
        g_nativeHostedSessionState.snapshot.sessionCompleted =
            sessionCompleted;
        g_nativeHostedSessionState.snapshot.requestedAppShutdown =
            requestedAppShutdown;
        g_nativeHostedSessionState.snapshot.shutdownHaltedGameplay =
            shutdownHaltedGameplay;
        g_nativeHostedSessionState.snapshot.gameplayLoopIterations =
            gameplayLoopIterations;
        g_nativeHostedSessionState.snapshot.appShutdownRequested =
            appShutdownRequested;
        g_nativeHostedSessionState.snapshot.gameplayHalted =
            gameplayHalted;
        g_nativeHostedSessionState.snapshot.stopSignalValid =
            stopSignalValid;
        g_nativeHostedSessionState.snapshot.active = false;
        g_nativeHostedSessionState.snapshot.runtimePhase =
            eDedicatedServerHostedGameRuntimePhase_Stopped;
        if (stoppedMs != 0)
        {
            g_nativeHostedSessionState.snapshot.stoppedMs = stoppedMs;
        }
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void FinalizeNativeDedicatedServerHostedGameSessionAndProject(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay,
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid,
        std::uint64_t stoppedMs,
        std::uint64_t nowMs)
    {
        FinalizeNativeDedicatedServerHostedGameSession(
            initialSaveRequested,
            initialSaveCompleted,
            initialSaveTimedOut,
            sessionCompleted,
            requestedAppShutdown,
            shutdownHaltedGameplay,
            gameplayLoopIterations,
            appShutdownRequested,
            gameplayHalted,
            stopSignalValid,
            stoppedMs);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionThreadState(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.hostedThreadActive =
            hostedThreadActive;
        g_nativeHostedSessionState.snapshot.hostedThreadTicks =
            hostedThreadTicks;
        RefreshNativeHostedSessionActive(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionThreadStateAndProject(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionThreadState(
            hostedThreadActive,
            hostedThreadTicks);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionThreadStateAndWorkerProject(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionThreadState(
            hostedThreadActive,
            hostedThreadTicks);
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionStartupTelemetry(
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.startupThreadIterations =
            startupThreadIterations;
        g_nativeHostedSessionState.snapshot.startupThreadDurationMs =
            startupThreadDurationMs;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionStartupTelemetry(
            startupThreadIterations,
            startupThreadDurationMs);
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void ObserveNativeDedicatedServerHostedGameSessionWorkerState(
        std::uint64_t pendingWorldActionTicks,
        std::uint64_t pendingAutosaveCommands,
        std::uint64_t pendingSaveCommands,
        std::uint64_t pendingStopCommands,
        std::uint64_t pendingHaltCommands,
        std::uint64_t workerTickCount,
        std::uint64_t completedWorkerActions,
        std::uint64_t processedAutosaveCommands,
        std::uint64_t processedSaveCommands,
        std::uint64_t processedStopCommands,
        std::uint64_t processedHaltCommands,
        std::uint64_t lastQueuedCommandId,
        std::uint64_t activeCommandId,
        std::uint64_t activeCommandTicksRemaining,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            activeCommandKind,
        std::uint64_t lastProcessedCommandId,
        ENativeDedicatedServerHostedGameWorkerCommandKind
            lastProcessedCommandKind)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        ApplyNativeHostedSessionWorkerSnapshot(
            &g_nativeHostedSessionState,
            NativeDedicatedServerHostedGameWorkerSnapshot{
                pendingWorldActionTicks,
                pendingAutosaveCommands,
                pendingSaveCommands,
                pendingStopCommands,
                pendingHaltCommands,
                workerTickCount,
                completedWorkerActions,
                processedAutosaveCommands,
                processedSaveCommands,
                processedStopCommands,
                processedHaltCommands,
                lastQueuedCommandId,
                activeCommandId,
                activeCommandTicksRemaining,
                activeCommandKind,
                lastProcessedCommandId,
                lastProcessedCommandKind});
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionWorkerSnapshot(
        const NativeDedicatedServerHostedGameWorkerSnapshot &workerSnapshot)
    {
        ObserveNativeDedicatedServerHostedGameSessionWorkerState(
            workerSnapshot.pendingWorldActionTicks,
            workerSnapshot.pendingAutosaveCommands,
            workerSnapshot.pendingSaveCommands,
            workerSnapshot.pendingStopCommands,
            workerSnapshot.pendingHaltCommands,
            workerSnapshot.workerTickCount,
            workerSnapshot.completedWorldActions,
            workerSnapshot.processedAutosaveCommands,
            workerSnapshot.processedSaveCommands,
            workerSnapshot.processedStopCommands,
            workerSnapshot.processedHaltCommands,
            workerSnapshot.lastQueuedCommandId,
            workerSnapshot.activeCommandId,
            workerSnapshot.activeCommandTicksRemaining,
            workerSnapshot.activeCommandKind,
            workerSnapshot.lastProcessedCommandId,
            workerSnapshot.lastProcessedCommandKind);
    }

    void ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionWorkerSnapshot(
            GetNativeDedicatedServerHostedGameWorkerSnapshot());
        ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(nowMs);
    }

    void RequestNativeDedicatedServerHostedGameSessionAutosave(
        unsigned int workTicks,
        std::uint64_t nowMs)
    {
        RequestNativeDedicatedServerHostedGameWorkerAutosave(workTicks);
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(nowMs);
    }

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionSaveCommand(
        std::uint64_t nowMs)
    {
        const std::uint64_t commandId =
            EnqueueNativeDedicatedServerHostedGameWorkerSaveCommand();
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(nowMs);
        return commandId;
    }

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionStopCommand(
        std::uint64_t nowMs)
    {
        const std::uint64_t commandId =
            EnqueueNativeDedicatedServerHostedGameWorkerStopCommand();
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(nowMs);
        return commandId;
    }

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionHaltSequence(
        bool requestAutosaveFirst,
        unsigned int autosaveWorkTicks,
        std::uint64_t nowMs)
    {
        const std::uint64_t commandId =
            EnqueueNativeDedicatedServerHostedGameWorkerHaltSequence(
                requestAutosaveFirst,
                autosaveWorkTicks);
        ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(nowMs);
        return commandId;
    }

    void ProjectNativeDedicatedServerHostedGameSessionSnapshotToRuntimeSnapshot(
        const NativeDedicatedServerHostedGameSessionSnapshot &snapshot,
        std::uint64_t nowMs)
    {
        DedicatedServerHostedGameRuntimePlanMetadata planMetadata = {};
        planMetadata.startAttempted = snapshot.startAttempted;
        planMetadata.loadedFromSave = snapshot.loadedFromSave;
        planMetadata.onlineGame = snapshot.onlineGame;
        planMetadata.privateGame = snapshot.privateGame;
        planMetadata.fakeLocalPlayerJoined =
            snapshot.fakeLocalPlayerJoined;
        planMetadata.resolvedSeed = snapshot.resolvedSeed;
        planMetadata.savePayloadBytes = snapshot.savePayloadBytes;
        planMetadata.savePayloadChecksum = snapshot.payloadChecksum;
        planMetadata.hostSettings = snapshot.hostSettings;
        planMetadata.dedicatedNoLocalHostPlayer =
            snapshot.dedicatedNoLocalHostPlayer;
        planMetadata.worldSizeChunks = snapshot.worldSizeChunks;
        planMetadata.worldHellScale = snapshot.worldHellScale;
        planMetadata.publicSlots = (unsigned char)snapshot.publicSlots;
        planMetadata.privateSlots = (unsigned char)snapshot.privateSlots;
        planMetadata.savePayloadName = snapshot.savePayloadName;
        planMetadata.loadedSaveMetadataAvailable =
            snapshot.loadedSaveMetadataAvailable;
        planMetadata.loadedSavePath = snapshot.loadedSavePath;
        planMetadata.previousStartupMode = snapshot.previousStartupMode;
        planMetadata.previousSessionPhase = snapshot.previousSessionPhase;
        planMetadata.previousRemoteCommands =
            snapshot.previousRemoteCommands;
        planMetadata.previousAutosaveCompletions =
            snapshot.previousAutosaveCompletions;
        planMetadata.previousWorkerPendingWorldActionTicks =
            snapshot.previousWorkerPendingWorldActionTicks;
        planMetadata.previousWorkerPendingAutosaveCommands =
            snapshot.previousWorkerPendingAutosaveCommands;
        planMetadata.previousWorkerPendingSaveCommands =
            snapshot.previousWorkerPendingSaveCommands;
        planMetadata.previousWorkerPendingStopCommands =
            snapshot.previousWorkerPendingStopCommands;
        planMetadata.previousWorkerPendingHaltCommands =
            snapshot.previousWorkerPendingHaltCommands;
        planMetadata.previousWorkerTickCount =
            snapshot.previousWorkerTickCount;
        planMetadata.previousCompletedWorkerActions =
            snapshot.previousCompletedWorkerActions;
        planMetadata.previousProcessedAutosaveCommands =
            snapshot.previousProcessedAutosaveCommands;
        planMetadata.previousProcessedSaveCommands =
            snapshot.previousProcessedSaveCommands;
        planMetadata.previousProcessedStopCommands =
            snapshot.previousProcessedStopCommands;
        planMetadata.previousProcessedHaltCommands =
            snapshot.previousProcessedHaltCommands;
        planMetadata.previousLastQueuedCommandId =
            snapshot.previousLastQueuedCommandId;
        planMetadata.previousActiveCommandId =
            snapshot.previousActiveCommandId;
        planMetadata.previousActiveCommandTicksRemaining =
            snapshot.previousActiveCommandTicksRemaining;
        planMetadata.previousActiveCommandKind =
            (unsigned int)snapshot.previousActiveCommandKind;
        planMetadata.previousLastProcessedCommandId =
            snapshot.previousLastProcessedCommandId;
        planMetadata.previousLastProcessedCommandKind =
            (unsigned int)snapshot.previousLastProcessedCommandKind;
        planMetadata.previousPlatformTickCount =
            snapshot.previousPlatformTickCount;
        planMetadata.previousUptimeMs = snapshot.previousUptimeMs;
        planMetadata.previousGameplayLoopIterations =
            snapshot.previousGameplayLoopIterations;
        planMetadata.previousHostSettings = snapshot.previousHostSettings;
        planMetadata.previousDedicatedNoLocalHostPlayer =
            snapshot.previousDedicatedNoLocalHostPlayer;
        planMetadata.previousWorldSizeChunks =
            snapshot.previousWorldSizeChunks;
        planMetadata.previousWorldHellScale =
            snapshot.previousWorldHellScale;
        planMetadata.previousOnlineGame = snapshot.previousOnlineGame;
        planMetadata.previousPrivateGame = snapshot.previousPrivateGame;
        planMetadata.previousFakeLocalPlayerJoined =
            snapshot.previousFakeLocalPlayerJoined;
        planMetadata.previousPublicSlots =
            snapshot.previousPublicSlots;
        planMetadata.previousPrivateSlots =
            snapshot.previousPrivateSlots;
        planMetadata.previousSavePayloadChecksum =
            snapshot.previousSavePayloadChecksum;
        planMetadata.previousSaveGeneration =
            snapshot.previousSaveGeneration;
        planMetadata.previousSessionStateChecksum =
            snapshot.previousSessionStateChecksum;
        planMetadata.previousStartupPayloadPresent =
            snapshot.previousStartupPayloadPresent;
        planMetadata.previousStartupPayloadValidated =
            snapshot.previousStartupPayloadValidated;
        planMetadata.previousStartupThreadIterations =
            snapshot.previousStartupThreadIterations;
        planMetadata.previousStartupThreadDurationMs =
            snapshot.previousStartupThreadDurationMs;
        planMetadata.previousHostedThreadActive =
            snapshot.previousHostedThreadActive;
        planMetadata.previousHostedThreadTicks =
            snapshot.previousHostedThreadTicks;
        planMetadata.previousSessionCompleted =
            snapshot.previousSessionCompleted;
        planMetadata.previousRequestedAppShutdown =
            snapshot.previousRequestedAppShutdown;
        planMetadata.previousShutdownHaltedGameplay =
            snapshot.previousShutdownHaltedGameplay;
        if (snapshot.startAttempted ||
            snapshot.loadedFromSave ||
            !snapshot.savePayloadName.empty() ||
            snapshot.hostSettings != 0 ||
            snapshot.loadedSaveMetadataAvailable ||
            !snapshot.loadedSavePath.empty())
        {
            RecordDedicatedServerHostedGameRuntimePlanMetadata(
                planMetadata);
        }

        if (nowMs != 0 &&
            (!snapshot.worldName.empty() ||
            !snapshot.worldSaveId.empty() ||
            !snapshot.savePath.empty() ||
            !snapshot.storageRoot.empty() ||
            !snapshot.hostName.empty() ||
            !snapshot.bindIp.empty() ||
            snapshot.configuredPort != 0 ||
            snapshot.listenerPort != 0))
        {
            DedicatedServerHostedGameRuntimeSessionContext sessionContext = {};
            sessionContext.worldName = snapshot.worldName;
            sessionContext.worldSaveId = snapshot.worldSaveId;
            sessionContext.savePath = snapshot.savePath;
            sessionContext.storageRoot = snapshot.storageRoot;
            sessionContext.hostName = snapshot.hostName;
            sessionContext.bindIp = snapshot.bindIp;
            sessionContext.configuredPort = snapshot.configuredPort;
            sessionContext.listenerPort = snapshot.listenerPort;
            RecordDedicatedServerHostedGameRuntimeSessionContext(
                sessionContext,
                snapshot.sessionStartMs);
        }

        if (nowMs != 0)
        {
            UpdateDedicatedServerHostedGameRuntimeSessionState(
                snapshot.acceptedConnections,
                snapshot.remoteCommands,
                snapshot.autosaveRequests,
                snapshot.observedAutosaveCompletions,
                snapshot.platformTickCount,
                snapshot.worldActionIdle,
                snapshot.appShutdownRequested,
                snapshot.gameplayHalted,
                snapshot.stopSignalValid,
                nowMs);
        }
        RecordDedicatedServerHostedGameRuntimeWorkerState(
            snapshot.workerPendingWorldActionTicks,
            snapshot.workerPendingAutosaveCommands,
            snapshot.workerPendingSaveCommands,
            snapshot.workerPendingStopCommands,
            snapshot.workerPendingHaltCommands,
            snapshot.workerTickCount,
            snapshot.completedWorkerActions,
            snapshot.processedAutosaveCommands,
            snapshot.processedSaveCommands,
            snapshot.processedStopCommands,
            snapshot.processedHaltCommands,
            snapshot.lastQueuedCommandId,
            snapshot.activeCommandId,
            snapshot.activeCommandTicksRemaining,
            (unsigned int)snapshot.activeCommandKind,
            snapshot.lastProcessedCommandId,
            (unsigned int)snapshot.lastProcessedCommandKind);
        RecordDedicatedServerHostedGameRuntimeThreadState(
            snapshot.hostedThreadActive,
            snapshot.hostedThreadTicks);
        RecordDedicatedServerHostedGameRuntimeGameplayLoopIteration(
            snapshot.gameplayLoopIterations);
        RecordDedicatedServerHostedGameRuntimeStartupResult(
            snapshot.startupResult,
            snapshot.threadInvoked);
        RecordDedicatedServerHostedGameRuntimeStartupTelemetry(
            snapshot.loadedFromSave,
            snapshot.payloadValidated,
            snapshot.startupThreadIterations,
            snapshot.startupThreadDurationMs);
        RecordDedicatedServerHostedGameRuntimeCoreState(
            snapshot.saveGeneration,
            snapshot.stateChecksum);
        RecordDedicatedServerHostedGameRuntimeCoreLifecycle(
            snapshot.active,
            (EDedicatedServerHostedGameRuntimePhase)snapshot.runtimePhase);
        DedicatedServerHostedGameRuntimeSessionSummary sessionSummary = {};
        sessionSummary.initialSaveRequested =
            snapshot.initialSaveRequested;
        sessionSummary.initialSaveCompleted =
            snapshot.initialSaveCompleted;
        sessionSummary.initialSaveTimedOut =
            snapshot.initialSaveTimedOut;
        sessionSummary.sessionCompleted =
            snapshot.sessionCompleted;
        sessionSummary.requestedAppShutdown =
            snapshot.requestedAppShutdown;
        sessionSummary.shutdownHaltedGameplay =
            snapshot.shutdownHaltedGameplay;
        sessionSummary.gameplayLoopIterations =
            snapshot.gameplayLoopIterations;
        RecordDedicatedServerHostedGameRuntimeSessionSummary(
            sessionSummary);
        if (!snapshot.lastPersistedSavePath.empty() ||
            snapshot.lastPersistedFileTime != 0 ||
            snapshot.lastPersistedAutosaveCompletions != 0)
        {
            RecordDedicatedServerHostedGameRuntimePersistedSave(
                !snapshot.lastPersistedSavePath.empty()
                    ? snapshot.lastPersistedSavePath
                    : snapshot.savePath,
                snapshot.lastPersistedFileTime,
                snapshot.lastPersistedAutosaveCompletions);
        }
        if (snapshot.stoppedMs != 0 ||
            snapshot.runtimePhase ==
                eDedicatedServerHostedGameRuntimePhase_Stopped)
        {
            MarkDedicatedServerHostedGameRuntimeSessionStopped(
                snapshot.stoppedMs);
        }
    }

    void ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
        std::uint64_t nowMs)
    {
        NativeDedicatedServerHostedGameSessionSnapshot snapshot = {};
        {
            std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
            snapshot = g_nativeHostedSessionState.snapshot;
        }

        ProjectNativeDedicatedServerHostedGameSessionSnapshotToRuntimeSnapshot(
            snapshot,
            nowMs);
    }

    void SignalNativeDedicatedServerHostedGameSessionThreadReady(
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionThreadStateAndWorkerProject(
            true,
            GetNativeDedicatedServerHostedGameSessionThreadTicks(),
            nowMs);
    }

    void SignalNativeDedicatedServerHostedGameSessionThreadStopped(
        std::uint64_t hostedThreadTicks,
        std::uint64_t nowMs)
    {
        ObserveNativeDedicatedServerHostedGameSessionThreadStateAndWorkerProject(
            false,
            hostedThreadTicks,
            nowMs);
    }

    void StopNativeDedicatedServerHostedGameSession(
        std::uint64_t stoppedMs)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.active = false;
        g_nativeHostedSessionState.snapshot.runtimePhase =
            eDedicatedServerHostedGameRuntimePhase_Stopped;
        if (stoppedMs != 0)
        {
            g_nativeHostedSessionState.snapshot.stoppedMs = stoppedMs;
        }
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    NativeDedicatedServerHostedGameSessionStopResult
    StopNativeDedicatedServerHostedGameSessionAndCaptureFinalState(
        std::uint64_t stoppedMs)
    {
        StopNativeDedicatedServerHostedGameSession(stoppedMs);

        NativeDedicatedServerHostedGameSessionStopResult result = {};
        result.workerSnapshot = GetNativeDedicatedServerHostedGameWorkerSnapshot();
        result.autosaveCompletions =
            GetNativeDedicatedServerHostedGameWorkerAutosaveCompletions();
        result.sessionSnapshot =
            GetNativeDedicatedServerHostedGameSessionSnapshot();
        return result;
    }

    bool IsNativeDedicatedServerHostedGameSessionRunning()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        return g_nativeHostedSessionState.snapshot.active &&
            g_nativeHostedSessionState.snapshot.hostedThreadActive;
    }

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        return g_nativeHostedSessionState.snapshot.hostedThreadTicks;
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        return g_nativeHostedSessionState.snapshot;
    }

    bool BuildNativeDedicatedServerSaveStubFromSessionSnapshot(
        const NativeDedicatedServerHostedGameSessionPersistContext
            &persistContext,
        std::uint64_t nowMs,
        std::uint64_t savedAtFileTime,
        NativeDedicatedServerSaveStub *outSaveStub)
    {
        if (outSaveStub == nullptr)
        {
            return false;
        }

        const NativeDedicatedServerHostedGameSessionSnapshot snapshot =
            GetNativeDedicatedServerHostedGameSessionSnapshot();
        NativeDedicatedServerSaveStub saveStub = {};
        saveStub.worldName = snapshot.worldName.empty()
            ? persistContext.worldName
            : snapshot.worldName;
        saveStub.levelId = snapshot.worldSaveId.empty()
            ? persistContext.worldSaveId
            : snapshot.worldSaveId;
        saveStub.hostName = snapshot.hostName.empty()
            ? persistContext.hostName
            : snapshot.hostName;
        saveStub.bindIp = snapshot.bindIp.empty()
            ? persistContext.bindIp
            : snapshot.bindIp;
        saveStub.configuredPort = snapshot.configuredPort > 0
            ? snapshot.configuredPort
            : persistContext.configuredPort;
        saveStub.listenerPort = snapshot.listenerPort > 0
            ? snapshot.listenerPort
            : persistContext.listenerPort;
        saveStub.onlineGame = snapshot.onlineGame;
        saveStub.privateGame = snapshot.privateGame;
        saveStub.fakeLocalPlayerJoined =
            snapshot.fakeLocalPlayerJoined;
        saveStub.publicSlots = snapshot.publicSlots;
        saveStub.privateSlots = snapshot.privateSlots;
        saveStub.sessionActive = snapshot.active;
        saveStub.worldActionIdle = snapshot.worldActionIdle;
        saveStub.appShutdownRequested =
            snapshot.appShutdownRequested;
        saveStub.gameplayHalted = snapshot.gameplayHalted;
        saveStub.acceptedConnections = snapshot.acceptedConnections;
        saveStub.remoteCommands = snapshot.remoteCommands;
        saveStub.autosaveRequests = snapshot.autosaveRequests;
        saveStub.autosaveCompletions =
            snapshot.observedAutosaveCompletions;
        saveStub.workerPendingWorldActionTicks =
            snapshot.workerPendingWorldActionTicks;
        saveStub.workerPendingAutosaveCommands =
            snapshot.workerPendingAutosaveCommands;
        saveStub.workerPendingSaveCommands =
            snapshot.workerPendingSaveCommands;
        saveStub.workerPendingStopCommands =
            snapshot.workerPendingStopCommands;
        saveStub.workerPendingHaltCommands =
            snapshot.workerPendingHaltCommands;
        saveStub.workerTickCount = snapshot.workerTickCount;
        saveStub.completedWorkerActions =
            snapshot.completedWorkerActions;
        saveStub.processedAutosaveCommands =
            snapshot.processedAutosaveCommands;
        saveStub.processedSaveCommands =
            snapshot.processedSaveCommands;
        saveStub.processedStopCommands =
            snapshot.processedStopCommands;
        saveStub.processedHaltCommands =
            snapshot.processedHaltCommands;
        saveStub.lastQueuedCommandId =
            snapshot.lastQueuedCommandId;
        saveStub.activeCommandId = snapshot.activeCommandId;
        saveStub.activeCommandTicksRemaining =
            snapshot.activeCommandTicksRemaining;
        saveStub.activeCommandKind = snapshot.activeCommandKind;
        saveStub.lastProcessedCommandId =
            snapshot.lastProcessedCommandId;
        saveStub.lastProcessedCommandKind =
            snapshot.lastProcessedCommandKind;
        saveStub.platformTickCount = snapshot.platformTickCount;
        if (snapshot.sessionStartMs != 0)
        {
            const std::uint64_t stoppedMs =
                snapshot.stoppedMs != 0
                    ? snapshot.stoppedMs
                    : nowMs;
            if (stoppedMs >= snapshot.sessionStartMs)
            {
                saveStub.uptimeMs =
                    stoppedMs - snapshot.sessionStartMs;
            }
        }
        saveStub.initialSaveRequested = snapshot.initialSaveRequested;
        saveStub.initialSaveCompleted = snapshot.initialSaveCompleted;
        saveStub.initialSaveTimedOut = snapshot.initialSaveTimedOut;
        saveStub.sessionCompleted = snapshot.sessionCompleted;
        saveStub.requestedAppShutdown =
            snapshot.requestedAppShutdown;
        saveStub.shutdownHaltedGameplay =
            snapshot.shutdownHaltedGameplay;
        saveStub.gameplayLoopIterations =
            snapshot.gameplayLoopIterations;
        saveStub.savedAtFileTime = savedAtFileTime;
        if (snapshot.startAttempted)
        {
            saveStub.startupMode =
                snapshot.loadedFromSave ? "loaded" : "created-new";
            saveStub.sessionPhase =
                GetDedicatedServerHostedGameRuntimePhaseName(
                    (EDedicatedServerHostedGameRuntimePhase)
                        snapshot.runtimePhase);
            saveStub.resolvedSeed = snapshot.resolvedSeed;
            saveStub.payloadBytes = snapshot.savePayloadBytes;
            saveStub.payloadChecksum = snapshot.payloadChecksum;
            saveStub.saveGeneration = snapshot.saveGeneration;
            saveStub.stateChecksum = snapshot.stateChecksum;
            saveStub.payloadName = snapshot.savePayloadName;
            saveStub.hostSettings = snapshot.hostSettings;
            saveStub.dedicatedNoLocalHostPlayer =
                snapshot.dedicatedNoLocalHostPlayer;
            saveStub.worldSizeChunks = snapshot.worldSizeChunks;
            saveStub.worldHellScale = snapshot.worldHellScale;
            saveStub.startupPayloadPresent =
                snapshot.savePayloadBytes > 0;
            saveStub.startupPayloadValidated =
                snapshot.payloadValidated;
            saveStub.startupThreadIterations =
                snapshot.startupThreadIterations;
            saveStub.startupThreadDurationMs =
                snapshot.startupThreadDurationMs;
            saveStub.hostedThreadActive =
                snapshot.hostedThreadActive;
            saveStub.hostedThreadTicks =
                snapshot.hostedThreadTicks;
        }

        *outSaveStub = saveStub;
        return true;
    }

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode)
    {
        if (outExitCode != nullptr)
        {
            *outExitCode = 0;
        }

        const bool stopped = WaitForNativeDedicatedServerHostedGameHostStop(
            timeoutMs,
            outExitCode);
        if (stopped)
        {
            const NativeDedicatedServerHostedGameSessionSnapshot snapshot =
                GetNativeDedicatedServerHostedGameSessionSnapshot();
            SignalNativeDedicatedServerHostedGameSessionThreadStopped(
                snapshot.hostedThreadTicks,
                LceGetMonotonicMilliseconds());
        }
        return stopped;
    }
}
