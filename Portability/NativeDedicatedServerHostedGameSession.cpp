#include "NativeDedicatedServerHostedGameSession.h"

#include <mutex>
#include <string>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "Minecraft.Server/Common/NativeDedicatedServerLoadedSaveState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "Minecraft.Server/Common/StringUtils.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameWorkerState();

    struct NativeDedicatedServerHostedGameSessionFrameInput
    {
        NativeDedicatedServerHostedGameWorkerSnapshot workerSnapshot = {};
        bool hostedThreadActive = true;
    };

    bool IsNativeDedicatedServerHostedGameSessionRunning();

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    NativeDedicatedServerHostedGameWorkerSnapshot
    GetNativeDedicatedServerHostedGameWorkerSnapshot();

    std::uint64_t GetNativeDedicatedServerHostedGameWorkerAutosaveCompletions();

    void RequestNativeDedicatedServerHostedGameWorkerAutosave(
        unsigned int workTicks);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerSaveCommand();

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerStopCommand();

    std::uint64_t EnqueueNativeDedicatedServerHostedGameWorkerHaltSequence(
        bool requestAutosaveFirst,
        unsigned int autosaveWorkTicks);

    void ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs);

    void ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs);

    void ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
        std::uint64_t nowMs = 0);

    void ProjectNativeDedicatedServerHostedGameSessionSnapshotToRuntimeSnapshot(
        const NativeDedicatedServerHostedGameSessionSnapshot &snapshot,
        std::uint64_t nowMs = 0);

    void ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
        std::uint64_t nowMs = 0);

    void ObserveNativeDedicatedServerHostedGameSessionPlatformState(
        std::uint64_t autosaveRequests,
        std::uint64_t platformTickCount);

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid);

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        const std::string &savePath,
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions);

    void RequestNativeDedicatedServerHostedGameSessionAutosave(
        unsigned int workTicks,
        std::uint64_t nowMs = 0);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionSaveCommand(
        std::uint64_t nowMs = 0);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionStopCommand(
        std::uint64_t nowMs = 0);

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionHaltSequence(
        bool requestAutosaveFirst,
        unsigned int autosaveWorkTicks,
        std::uint64_t nowMs = 0);

    bool BuildNativeDedicatedServerSaveStubFromSessionSnapshot(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
        std::uint64_t nowMs,
        std::uint64_t savedAtFileTime,
        NativeDedicatedServerSaveStub *outSaveStub);

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);

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
                state->snapshot.context.worldName);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.context.worldSaveId);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.context.savePath);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.context.storageRoot);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.context.hostName);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.context.bindIp);
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.startup.result));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.runtimePhase));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(state->snapshot.localUsersMask));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(
                    state->snapshot.context.configuredPort));
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(
                    state->snapshot.context.listenerPort));
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.activation.publicSlots);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.activation.privateSlots);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.payload.checksum);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.sessionTicks);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.observedAutosaveCompletions);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.pendingWorldActionTicks);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.pendingAutosaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.pendingSaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.pendingStopCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.pendingHaltCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.workerTickCount);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.completedWorldActions);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.processedAutosaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.processedSaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.processedStopCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.processedHaltCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.lastQueuedCommandId);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.activeCommandId);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.activeCommandTicksRemaining);
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(
                    state->snapshot.worker.activeCommandKind));
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.worker.lastProcessedCommandId);
            checksum = MixNativeHostedSessionHash(
                checksum,
                static_cast<std::uint64_t>(
                    state->snapshot.worker.lastProcessedCommandKind));
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
                state->snapshot.persistedSave.fileTime);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.persistedSave.autosaveCompletions);
            checksum = MixNativeHostedSessionStringHash(
                checksum,
                state->snapshot.persistedSave.savePath);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.startup.threadIterations);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.startup.threadDurationMs);
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
                state->snapshot.startup.payloadValidated ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.threadInvoked ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.activation.onlineGame ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.activation.privateGame ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.activation.fakeLocalPlayerJoined ? 1U : 0U);
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
                state->snapshot.summary.initialSaveRequested ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.summary.initialSaveCompleted ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.summary.initialSaveTimedOut ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.summary.sessionCompleted ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.summary.requestedAppShutdown ? 1U : 0U);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.summary.shutdownHaltedGameplay ? 1U : 0U);
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

            state->snapshot.worker.pendingWorldActionTicks =
                workerSnapshot.pendingWorldActionTicks;
            state->snapshot.worker.pendingAutosaveCommands =
                workerSnapshot.pendingAutosaveCommands;
            state->snapshot.worker.pendingSaveCommands =
                workerSnapshot.pendingSaveCommands;
            state->snapshot.worker.pendingStopCommands =
                workerSnapshot.pendingStopCommands;
            state->snapshot.worker.pendingHaltCommands =
                workerSnapshot.pendingHaltCommands;
            state->snapshot.worker.workerTickCount =
                state->baseWorkerTickCount +
                workerSnapshot.workerTickCount;
            state->snapshot.worker.completedWorldActions =
                state->baseCompletedWorkerActions +
                workerSnapshot.completedWorldActions;
            state->snapshot.worker.processedAutosaveCommands =
                state->baseProcessedAutosaveCommands +
                workerSnapshot.processedAutosaveCommands;
            state->snapshot.worker.processedSaveCommands =
                state->baseProcessedSaveCommands +
                workerSnapshot.processedSaveCommands;
            state->snapshot.worker.processedStopCommands =
                state->baseProcessedStopCommands +
                workerSnapshot.processedStopCommands;
            state->snapshot.worker.processedHaltCommands =
                state->baseProcessedHaltCommands +
                workerSnapshot.processedHaltCommands;
            state->snapshot.worker.lastQueuedCommandId =
                workerSnapshot.lastQueuedCommandId;
            state->snapshot.worker.activeCommandId =
                workerSnapshot.activeCommandId;
            state->snapshot.worker.activeCommandTicksRemaining =
                workerSnapshot.activeCommandTicksRemaining;
            state->snapshot.worker.activeCommandKind =
                workerSnapshot.activeCommandKind;
            state->snapshot.worker.lastProcessedCommandId =
                workerSnapshot.lastProcessedCommandId;
            state->snapshot.worker.lastProcessedCommandKind =
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

        void SetNativeHostedSessionStartupValidationState(
            NativeDedicatedServerHostedGameSessionState *state,
            bool startupPayloadValidated)
        {
            if (state == nullptr)
            {
                return;
            }

            state->snapshot.active = startupPayloadValidated;
            state->snapshot.startup.payloadValidated =
                startupPayloadValidated;
            state->snapshot.runtimePhase =
                startupPayloadValidated
                    ? eDedicatedServerHostedGameRuntimePhase_Startup
                    : eDedicatedServerHostedGameRuntimePhase_Failed;
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

            if (state->snapshot.startup.result != 0)
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
                state->snapshot.startup.payloadValidated ||
                state->snapshot.startup.result == 0)
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

            if (state->snapshot.startup.result != 0)
            {
                state->snapshot.active = false;
                return;
            }

            if (state->snapshot.hostedThreadActive ||
                state->snapshot.threadInvoked ||
                state->snapshot.startup.payloadValidated ||
                state->snapshot.startup.result == 0)
            {
                state->snapshot.active = true;
            }
        }

        void SetNativeHostedSessionStoppedState(
            NativeDedicatedServerHostedGameSessionState *state,
            std::uint64_t stoppedMs)
        {
            if (state == nullptr)
            {
                return;
            }

            state->snapshot.active = false;
            state->snapshot.runtimePhase =
                eDedicatedServerHostedGameRuntimePhase_Stopped;
            if (stoppedMs != 0)
            {
                state->snapshot.stoppedMs = stoppedMs;
            }

            RefreshNativeHostedSessionPhase(state);
            RefreshNativeHostedSessionStateChecksum(state);
        }

        int BuildNativeHostedSessionStartupResult(
            const NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
            bool startupPayloadValidated)
        {
            return initData != nullptr && startupPayloadValidated ? 0 : -2;
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
        g_nativeHostedSessionState.snapshot.loadedFromSave =
            initData.saveData != nullptr;
        g_nativeHostedSessionState.snapshot.resolvedSeed = initData.seed;
        g_nativeHostedSessionState.snapshot.worldConfig.hostSettings =
            initData.settings;
        g_nativeHostedSessionState.snapshot.localUsersMask =
            initData.localUsersMask;
        g_nativeHostedSessionState.snapshot.activation.onlineGame =
            initData.onlineGame;
        g_nativeHostedSessionState.snapshot.activation.privateGame =
            initData.privateGame;
        g_nativeHostedSessionState.snapshot.activation.publicSlots =
            initData.publicSlots;
        g_nativeHostedSessionState.snapshot.activation.privateSlots =
            initData.privateSlots;
        g_nativeHostedSessionState.snapshot.activation.fakeLocalPlayerJoined =
            initData.fakeLocalPlayerJoined;
        g_nativeHostedSessionState.snapshot
            .worldConfig.dedicatedNoLocalHostPlayer =
            initData.dedicatedNoLocalHostPlayer;
        g_nativeHostedSessionState.snapshot.worldConfig.worldSizeChunks =
            initData.xzSize;
        g_nativeHostedSessionState.snapshot.worldConfig.worldHellScale =
            initData.hellScale;
        SetNativeHostedSessionStartupValidationState(
            &g_nativeHostedSessionState,
            startupPayloadValidated);
        g_nativeHostedSessionState.snapshot.payload.checksum =
            ComputeNativeHostedSessionBytesChecksum(
                initData.saveData != nullptr ? initData.saveData->data : nullptr,
                initData.saveData != nullptr ? initData.saveData->fileSize : 0);

        if (initData.saveData != nullptr &&
            initData.saveData->data != nullptr &&
            initData.saveData->fileSize > 0)
        {
            g_nativeHostedSessionState.snapshot.payload.bytes =
                initData.saveData->fileSize;
            g_nativeHostedSessionState.snapshot.payload.name =
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
                g_nativeHostedSessionState.snapshot
                    .worker.pendingWorldActionTicks =
                        saveStub.workerPendingWorldActionTicks;
                g_nativeHostedSessionState.snapshot.worker.workerTickCount =
                    saveStub.workerTickCount;
                g_nativeHostedSessionState.snapshot.worker.completedWorldActions =
                    saveStub.completedWorkerActions;
                g_nativeHostedSessionState.snapshot
                    .worker.pendingAutosaveCommands =
                        saveStub.workerPendingAutosaveCommands;
                g_nativeHostedSessionState.snapshot
                    .worker.pendingSaveCommands =
                        saveStub.workerPendingSaveCommands;
                g_nativeHostedSessionState.snapshot
                    .worker.pendingStopCommands =
                        saveStub.workerPendingStopCommands;
                g_nativeHostedSessionState.snapshot
                    .worker.processedAutosaveCommands =
                        saveStub.processedAutosaveCommands;
                g_nativeHostedSessionState.snapshot.worker.processedSaveCommands =
                    saveStub.processedSaveCommands;
                g_nativeHostedSessionState.snapshot.worker.processedStopCommands =
                    saveStub.processedStopCommands;
                g_nativeHostedSessionState.snapshot.worker.pendingHaltCommands =
                    saveStub.workerPendingHaltCommands;
                g_nativeHostedSessionState.snapshot.worker.processedHaltCommands =
                    saveStub.processedHaltCommands;
                g_nativeHostedSessionState.snapshot.worker.lastQueuedCommandId =
                    saveStub.lastQueuedCommandId;
                g_nativeHostedSessionState.snapshot.worker.activeCommandId =
                    saveStub.activeCommandId;
                g_nativeHostedSessionState.snapshot
                    .worker.activeCommandTicksRemaining =
                        saveStub.activeCommandTicksRemaining;
                g_nativeHostedSessionState.snapshot.worker.activeCommandKind =
                    (ENativeDedicatedServerHostedGameWorkerCommandKind)
                        saveStub.activeCommandKind;
                g_nativeHostedSessionState.snapshot.worker.lastProcessedCommandId =
                    saveStub.lastProcessedCommandId;
                g_nativeHostedSessionState.snapshot.worker.lastProcessedCommandKind =
                    (ENativeDedicatedServerHostedGameWorkerCommandKind)
                        saveStub.lastProcessedCommandKind;
                g_nativeHostedSessionState.snapshot.gameplayLoopIterations =
                    saveStub.gameplayLoopIterations;
                g_nativeHostedSessionState.snapshot.persistedSave.fileTime =
                    saveStub.savedAtFileTime;
                g_nativeHostedSessionState.snapshot
                    .persistedSave.autosaveCompletions =
                    saveStub.autosaveCompletions;
            }
        }

        const NativeDedicatedServerLoadedSaveMetadata loadedSaveMetadata =
            GetNativeDedicatedServerLoadedSaveMetadata();
        if (loadedSaveMetadata.available)
        {
            g_nativeHostedSessionState.snapshot.loadedSave.path =
                loadedSaveMetadata.savePath;
        }
        if (loadedSaveMetadata.hasSaveStub)
        {
            g_nativeHostedSessionState.snapshot
                .loadedSave.metadataAvailable = true;
            g_nativeHostedSessionState.snapshot.previousStartup.mode =
                loadedSaveMetadata.saveStub.startupMode;
            g_nativeHostedSessionState.snapshot.previousSessionPhase =
                loadedSaveMetadata.saveStub.sessionPhase;
            g_nativeHostedSessionState.snapshot.previousRemoteCommands =
                loadedSaveMetadata.saveStub.remoteCommands;
            g_nativeHostedSessionState.snapshot
                .previousAutosaveCompletions =
                    loadedSaveMetadata.saveStub.autosaveCompletions;
            g_nativeHostedSessionState.snapshot
                .previousWorker.pendingWorldActionTicks =
                    loadedSaveMetadata.saveStub
                        .workerPendingWorldActionTicks;
            g_nativeHostedSessionState.snapshot
                .previousWorker.pendingAutosaveCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingAutosaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorker.pendingSaveCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingSaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorker.pendingStopCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingStopCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorker.pendingHaltCommands =
                    loadedSaveMetadata.saveStub
                        .workerPendingHaltCommands;
            g_nativeHostedSessionState.snapshot.previousWorker.workerTickCount =
                loadedSaveMetadata.saveStub.workerTickCount;
            g_nativeHostedSessionState.snapshot
                .previousWorker.completedWorldActions =
                    loadedSaveMetadata.saveStub.completedWorkerActions;
            g_nativeHostedSessionState.snapshot
                .previousWorker.processedAutosaveCommands =
                    loadedSaveMetadata.saveStub
                        .processedAutosaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorker.processedSaveCommands =
                    loadedSaveMetadata.saveStub.processedSaveCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorker.processedStopCommands =
                    loadedSaveMetadata.saveStub.processedStopCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorker.processedHaltCommands =
                    loadedSaveMetadata.saveStub.processedHaltCommands;
            g_nativeHostedSessionState.snapshot
                .previousWorker.lastQueuedCommandId =
                    loadedSaveMetadata.saveStub.lastQueuedCommandId;
            g_nativeHostedSessionState.snapshot.previousWorker.activeCommandId =
                loadedSaveMetadata.saveStub.activeCommandId;
            g_nativeHostedSessionState.snapshot
                .previousWorker.activeCommandTicksRemaining =
                    loadedSaveMetadata.saveStub
                        .activeCommandTicksRemaining;
            g_nativeHostedSessionState.snapshot.previousWorker.activeCommandKind =
                (ENativeDedicatedServerHostedGameWorkerCommandKind)
                    loadedSaveMetadata.saveStub.activeCommandKind;
            g_nativeHostedSessionState.snapshot
                .previousWorker.lastProcessedCommandId =
                    loadedSaveMetadata.saveStub.lastProcessedCommandId;
            g_nativeHostedSessionState.snapshot
                .previousWorker.lastProcessedCommandKind =
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
            g_nativeHostedSessionState.snapshot.previousWorldConfig.hostSettings =
                loadedSaveMetadata.saveStub.hostSettings;
            g_nativeHostedSessionState.snapshot
                .previousWorldConfig.dedicatedNoLocalHostPlayer =
                    loadedSaveMetadata.saveStub
                        .dedicatedNoLocalHostPlayer;
            g_nativeHostedSessionState.snapshot
                .previousWorldConfig.worldSizeChunks =
                    loadedSaveMetadata.saveStub.worldSizeChunks;
            g_nativeHostedSessionState.snapshot
                .previousWorldConfig.worldHellScale =
                (unsigned char)
                    loadedSaveMetadata.saveStub.worldHellScale;
            g_nativeHostedSessionState.snapshot.previousActivation.onlineGame =
                loadedSaveMetadata.saveStub.onlineGame;
            g_nativeHostedSessionState.snapshot.previousActivation.privateGame =
                loadedSaveMetadata.saveStub.privateGame;
            g_nativeHostedSessionState.snapshot
                .previousActivation.fakeLocalPlayerJoined =
                    loadedSaveMetadata.saveStub.fakeLocalPlayerJoined;
            g_nativeHostedSessionState.snapshot.previousActivation.publicSlots =
                (unsigned char)loadedSaveMetadata.saveStub.publicSlots;
            g_nativeHostedSessionState.snapshot.previousActivation.privateSlots =
                (unsigned char)loadedSaveMetadata.saveStub.privateSlots;
            g_nativeHostedSessionState.snapshot
                .previousPayload.checksum =
                    loadedSaveMetadata.saveStub.payloadChecksum;
            g_nativeHostedSessionState.snapshot.previousSaveGeneration =
                loadedSaveMetadata.saveStub.saveGeneration;
            g_nativeHostedSessionState.snapshot
                .previousSessionStateChecksum =
                    loadedSaveMetadata.saveStub.stateChecksum;
            g_nativeHostedSessionState.snapshot
                .previousStartup.payloadPresent =
                    loadedSaveMetadata.saveStub
                        .startupPayloadPresent;
            g_nativeHostedSessionState.snapshot
                .previousStartup.payloadValidated =
                    loadedSaveMetadata.saveStub
                        .startupPayloadValidated;
            g_nativeHostedSessionState.snapshot
                .previousStartup.threadIterations =
                    loadedSaveMetadata.saveStub
                        .startupThreadIterations;
            g_nativeHostedSessionState.snapshot
                .previousStartup.threadDurationMs =
                    loadedSaveMetadata.saveStub
                        .startupThreadDurationMs;
            g_nativeHostedSessionState.snapshot
                .previousHostedThreadActive =
                    loadedSaveMetadata.saveStub.hostedThreadActive;
            g_nativeHostedSessionState.snapshot.previousHostedThreadTicks =
                loadedSaveMetadata.saveStub.hostedThreadTicks;
            g_nativeHostedSessionState.snapshot
                .previousSummary.initialSaveRequested =
                    loadedSaveMetadata.saveStub.initialSaveRequested;
            g_nativeHostedSessionState.snapshot
                .previousSummary.initialSaveCompleted =
                    loadedSaveMetadata.saveStub.initialSaveCompleted;
            g_nativeHostedSessionState.snapshot
                .previousSummary.initialSaveTimedOut =
                    loadedSaveMetadata.saveStub.initialSaveTimedOut;
            g_nativeHostedSessionState.snapshot
                .previousSummary.sessionCompleted =
                    loadedSaveMetadata.saveStub.sessionCompleted;
            g_nativeHostedSessionState.snapshot
                .previousSummary.requestedAppShutdown =
                    loadedSaveMetadata.saveStub.requestedAppShutdown;
            g_nativeHostedSessionState.snapshot
                .previousSummary.shutdownHaltedGameplay =
                    loadedSaveMetadata.saveStub.shutdownHaltedGameplay;
        }

        RefreshNativeHostedSessionActive(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionPhase(&g_nativeHostedSessionState);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
        return startupPayloadValidated;
    }

    bool StartNativeDedicatedServerHostedGameSessionAndProjectStartup(
        const NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs)
    {
        const bool startupPayloadValidated =
            initData != nullptr &&
            StartNativeDedicatedServerHostedGameSession(*initData);
        const int startupResult = BuildNativeHostedSessionStartupResult(
            initData,
            startupPayloadValidated);
        ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
            startupThreadIterations,
            startupThreadDurationMs,
            nowMs);
        ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
            startupResult,
            false,
            nowMs);
        return startupPayloadValidated;
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
        const NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs)
    {
        (void)StartNativeDedicatedServerHostedGameSessionAndProjectStartup(
            initData,
            startupThreadIterations,
            startupThreadDurationMs,
            nowMs);
        return GetNativeDedicatedServerHostedGameSessionSnapshot();
    }

    void ObserveNativeDedicatedServerHostedGameSessionStartupResult(
        int startupResult,
        bool threadInvoked)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.startup.result = startupResult;
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
            GetNativeDedicatedServerHostedGameWorkerAutosaveCompletions();
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

    void TickNativeDedicatedServerHostedGameSessionWorkerFrameAndProject(
        const NativeDedicatedServerHostedGameWorkerSnapshot &workerSnapshot,
        bool hostedThreadActive,
        std::uint64_t nowMs)
    {
        NativeDedicatedServerHostedGameSessionFrameInput frameInput = {};
        frameInput.workerSnapshot = workerSnapshot;
        frameInput.hostedThreadActive = hostedThreadActive;
        TickNativeDedicatedServerHostedGameSessionFrameAndProject(
            frameInput,
            nowMs);
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
        g_nativeHostedSessionState.snapshot.context.worldName = worldName;
        g_nativeHostedSessionState.snapshot.context.worldSaveId = worldSaveId;
        g_nativeHostedSessionState.snapshot.context.savePath = savePath;
        g_nativeHostedSessionState.snapshot.context.storageRoot = storageRoot;
        g_nativeHostedSessionState.snapshot.context.hostName = hostName;
        g_nativeHostedSessionState.snapshot.context.bindIp = bindIp;
        g_nativeHostedSessionState.snapshot.context.configuredPort =
            configuredPort;
        g_nativeHostedSessionState.snapshot.context.listenerPort =
            listenerPort;
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
        g_nativeHostedSessionState.snapshot.activation.onlineGame =
            onlineGame;
        g_nativeHostedSessionState.snapshot.activation.privateGame =
            privateGame;
        g_nativeHostedSessionState.snapshot.activation.publicSlots =
            publicSlots;
        g_nativeHostedSessionState.snapshot.activation.privateSlots =
            privateSlots;
        g_nativeHostedSessionState.snapshot.activation.fakeLocalPlayerJoined =
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
            g_nativeHostedSessionState.snapshot.persistedSave.savePath =
                savePath;
        }
        g_nativeHostedSessionState.snapshot.persistedSave.fileTime =
            savedAtFileTime;
        g_nativeHostedSessionState.snapshot
            .persistedSave.autosaveCompletions = autosaveCompletions;
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
        g_nativeHostedSessionState.snapshot.summary.initialSaveRequested =
            initialSaveRequested;
        g_nativeHostedSessionState.snapshot.summary.initialSaveCompleted =
            initialSaveCompleted;
        g_nativeHostedSessionState.snapshot.summary.initialSaveTimedOut =
            initialSaveTimedOut;
        g_nativeHostedSessionState.snapshot.summary.sessionCompleted =
            sessionCompleted;
        g_nativeHostedSessionState.snapshot.summary.requestedAppShutdown =
            requestedAppShutdown;
        g_nativeHostedSessionState.snapshot.summary.shutdownHaltedGameplay =
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
        g_nativeHostedSessionState.snapshot.summary.initialSaveRequested =
            initialSaveRequested;
        g_nativeHostedSessionState.snapshot.summary.initialSaveCompleted =
            initialSaveCompleted;
        g_nativeHostedSessionState.snapshot.summary.initialSaveTimedOut =
            initialSaveTimedOut;
        g_nativeHostedSessionState.snapshot.summary.sessionCompleted =
            sessionCompleted;
        g_nativeHostedSessionState.snapshot.summary.requestedAppShutdown =
            requestedAppShutdown;
        g_nativeHostedSessionState.snapshot.summary.shutdownHaltedGameplay =
            shutdownHaltedGameplay;
        g_nativeHostedSessionState.snapshot.gameplayLoopIterations =
            gameplayLoopIterations;
        g_nativeHostedSessionState.snapshot.appShutdownRequested =
            appShutdownRequested;
        g_nativeHostedSessionState.snapshot.gameplayHalted =
            gameplayHalted;
        g_nativeHostedSessionState.snapshot.stopSignalValid =
            stopSignalValid;
        SetNativeHostedSessionStoppedState(
            &g_nativeHostedSessionState,
            stoppedMs);
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
        g_nativeHostedSessionState.snapshot.startup.threadIterations =
            startupThreadIterations;
        g_nativeHostedSessionState.snapshot.startup.threadDurationMs =
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

    void ObserveNativeDedicatedServerHostedGameSessionWorkerSnapshot(
        const NativeDedicatedServerHostedGameWorkerSnapshot &workerSnapshot)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        ApplyNativeHostedSessionWorkerSnapshot(
            &g_nativeHostedSessionState,
            workerSnapshot);
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
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
        planMetadata.onlineGame = snapshot.activation.onlineGame;
        planMetadata.privateGame = snapshot.activation.privateGame;
        planMetadata.fakeLocalPlayerJoined =
            snapshot.activation.fakeLocalPlayerJoined;
        planMetadata.resolvedSeed = snapshot.resolvedSeed;
        planMetadata.savePayloadBytes = snapshot.payload.bytes;
        planMetadata.savePayloadChecksum = snapshot.payload.checksum;
        planMetadata.hostSettings = snapshot.worldConfig.hostSettings;
        planMetadata.dedicatedNoLocalHostPlayer =
            snapshot.worldConfig.dedicatedNoLocalHostPlayer;
        planMetadata.worldSizeChunks =
            snapshot.worldConfig.worldSizeChunks;
        planMetadata.worldHellScale =
            snapshot.worldConfig.worldHellScale;
        planMetadata.publicSlots =
            (unsigned char)snapshot.activation.publicSlots;
        planMetadata.privateSlots =
            (unsigned char)snapshot.activation.privateSlots;
        planMetadata.savePayloadName = snapshot.payload.name;
        planMetadata.loadedSaveMetadataAvailable =
            snapshot.loadedSave.metadataAvailable;
        planMetadata.loadedSavePath = snapshot.loadedSave.path;
        planMetadata.previousStartupMode = snapshot.previousStartup.mode;
        planMetadata.previousSessionPhase = snapshot.previousSessionPhase;
        planMetadata.previousRemoteCommands =
            snapshot.previousRemoteCommands;
        planMetadata.previousAutosaveCompletions =
            snapshot.previousAutosaveCompletions;
        planMetadata.previousWorkerPendingWorldActionTicks =
            snapshot.previousWorker.pendingWorldActionTicks;
        planMetadata.previousWorkerPendingAutosaveCommands =
            snapshot.previousWorker.pendingAutosaveCommands;
        planMetadata.previousWorkerPendingSaveCommands =
            snapshot.previousWorker.pendingSaveCommands;
        planMetadata.previousWorkerPendingStopCommands =
            snapshot.previousWorker.pendingStopCommands;
        planMetadata.previousWorkerPendingHaltCommands =
            snapshot.previousWorker.pendingHaltCommands;
        planMetadata.previousWorkerTickCount =
            snapshot.previousWorker.workerTickCount;
        planMetadata.previousCompletedWorkerActions =
            snapshot.previousWorker.completedWorldActions;
        planMetadata.previousProcessedAutosaveCommands =
            snapshot.previousWorker.processedAutosaveCommands;
        planMetadata.previousProcessedSaveCommands =
            snapshot.previousWorker.processedSaveCommands;
        planMetadata.previousProcessedStopCommands =
            snapshot.previousWorker.processedStopCommands;
        planMetadata.previousProcessedHaltCommands =
            snapshot.previousWorker.processedHaltCommands;
        planMetadata.previousLastQueuedCommandId =
            snapshot.previousWorker.lastQueuedCommandId;
        planMetadata.previousActiveCommandId =
            snapshot.previousWorker.activeCommandId;
        planMetadata.previousActiveCommandTicksRemaining =
            snapshot.previousWorker.activeCommandTicksRemaining;
        planMetadata.previousActiveCommandKind =
            (unsigned int)snapshot.previousWorker.activeCommandKind;
        planMetadata.previousLastProcessedCommandId =
            snapshot.previousWorker.lastProcessedCommandId;
        planMetadata.previousLastProcessedCommandKind =
            (unsigned int)snapshot.previousWorker.lastProcessedCommandKind;
        planMetadata.previousPlatformTickCount =
            snapshot.previousPlatformTickCount;
        planMetadata.previousUptimeMs = snapshot.previousUptimeMs;
        planMetadata.previousGameplayLoopIterations =
            snapshot.previousGameplayLoopIterations;
        planMetadata.previousHostSettings =
            snapshot.previousWorldConfig.hostSettings;
        planMetadata.previousDedicatedNoLocalHostPlayer =
            snapshot.previousWorldConfig.dedicatedNoLocalHostPlayer;
        planMetadata.previousWorldSizeChunks =
            snapshot.previousWorldConfig.worldSizeChunks;
        planMetadata.previousWorldHellScale =
            snapshot.previousWorldConfig.worldHellScale;
        planMetadata.previousOnlineGame =
            snapshot.previousActivation.onlineGame;
        planMetadata.previousPrivateGame =
            snapshot.previousActivation.privateGame;
        planMetadata.previousFakeLocalPlayerJoined =
            snapshot.previousActivation.fakeLocalPlayerJoined;
        planMetadata.previousPublicSlots =
            (unsigned char)snapshot.previousActivation.publicSlots;
        planMetadata.previousPrivateSlots =
            (unsigned char)snapshot.previousActivation.privateSlots;
        planMetadata.previousSavePayloadChecksum =
            snapshot.previousPayload.checksum;
        planMetadata.previousSaveGeneration =
            snapshot.previousSaveGeneration;
        planMetadata.previousSessionStateChecksum =
            snapshot.previousSessionStateChecksum;
        planMetadata.previousStartupPayloadPresent =
            snapshot.previousStartup.payloadPresent;
        planMetadata.previousStartupPayloadValidated =
            snapshot.previousStartup.payloadValidated;
        planMetadata.previousStartupThreadIterations =
            snapshot.previousStartup.threadIterations;
        planMetadata.previousStartupThreadDurationMs =
            snapshot.previousStartup.threadDurationMs;
        planMetadata.previousHostedThreadActive =
            snapshot.previousHostedThreadActive;
        planMetadata.previousHostedThreadTicks =
            snapshot.previousHostedThreadTicks;
        planMetadata.previousSessionCompleted =
            snapshot.previousSummary.sessionCompleted;
        planMetadata.previousRequestedAppShutdown =
            snapshot.previousSummary.requestedAppShutdown;
        planMetadata.previousShutdownHaltedGameplay =
            snapshot.previousSummary.shutdownHaltedGameplay;
        if (snapshot.startAttempted ||
            snapshot.loadedFromSave ||
            !snapshot.payload.name.empty() ||
            snapshot.worldConfig.hostSettings != 0 ||
            snapshot.loadedSave.metadataAvailable ||
            !snapshot.loadedSave.path.empty())
        {
            RecordDedicatedServerHostedGameRuntimePlanMetadata(
                planMetadata);
        }

        if (nowMs != 0 &&
            (!snapshot.context.worldName.empty() ||
            !snapshot.context.worldSaveId.empty() ||
            !snapshot.context.savePath.empty() ||
            !snapshot.context.storageRoot.empty() ||
            !snapshot.context.hostName.empty() ||
            !snapshot.context.bindIp.empty() ||
            snapshot.context.configuredPort != 0 ||
            snapshot.context.listenerPort != 0))
        {
            DedicatedServerHostedGameRuntimeSessionContext sessionContext = {};
            sessionContext.worldName = snapshot.context.worldName;
            sessionContext.worldSaveId = snapshot.context.worldSaveId;
            sessionContext.savePath = snapshot.context.savePath;
            sessionContext.storageRoot = snapshot.context.storageRoot;
            sessionContext.hostName = snapshot.context.hostName;
            sessionContext.bindIp = snapshot.context.bindIp;
            sessionContext.configuredPort =
                snapshot.context.configuredPort;
            sessionContext.listenerPort =
                snapshot.context.listenerPort;
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
            snapshot.worker.pendingWorldActionTicks,
            snapshot.worker.pendingAutosaveCommands,
            snapshot.worker.pendingSaveCommands,
            snapshot.worker.pendingStopCommands,
            snapshot.worker.pendingHaltCommands,
            snapshot.worker.workerTickCount,
            snapshot.worker.completedWorldActions,
            snapshot.worker.processedAutosaveCommands,
            snapshot.worker.processedSaveCommands,
            snapshot.worker.processedStopCommands,
            snapshot.worker.processedHaltCommands,
            snapshot.worker.lastQueuedCommandId,
            snapshot.worker.activeCommandId,
            snapshot.worker.activeCommandTicksRemaining,
            (unsigned int)snapshot.worker.activeCommandKind,
            snapshot.worker.lastProcessedCommandId,
            (unsigned int)snapshot.worker.lastProcessedCommandKind);
        RecordDedicatedServerHostedGameRuntimeThreadState(
            snapshot.hostedThreadActive,
            snapshot.hostedThreadTicks);
        RecordDedicatedServerHostedGameRuntimeGameplayLoopIteration(
            snapshot.gameplayLoopIterations);
        RecordDedicatedServerHostedGameRuntimeStartupResult(
            snapshot.startup.result,
            snapshot.threadInvoked);
        RecordDedicatedServerHostedGameRuntimeStartupTelemetry(
            snapshot.loadedFromSave,
            snapshot.startup.payloadValidated,
            snapshot.startup.threadIterations,
            snapshot.startup.threadDurationMs);
        RecordDedicatedServerHostedGameRuntimeCoreState(
            snapshot.saveGeneration,
            snapshot.stateChecksum);
        RecordDedicatedServerHostedGameRuntimeCoreLifecycle(
            snapshot.active,
            (EDedicatedServerHostedGameRuntimePhase)snapshot.runtimePhase);
        DedicatedServerHostedGameRuntimeSessionSummary sessionSummary = {};
        sessionSummary.initialSaveRequested =
            snapshot.summary.initialSaveRequested;
        sessionSummary.initialSaveCompleted =
            snapshot.summary.initialSaveCompleted;
        sessionSummary.initialSaveTimedOut =
            snapshot.summary.initialSaveTimedOut;
        sessionSummary.sessionCompleted =
            snapshot.summary.sessionCompleted;
        sessionSummary.requestedAppShutdown =
            snapshot.summary.requestedAppShutdown;
        sessionSummary.shutdownHaltedGameplay =
            snapshot.summary.shutdownHaltedGameplay;
        sessionSummary.gameplayLoopIterations =
            snapshot.gameplayLoopIterations;
        RecordDedicatedServerHostedGameRuntimeSessionSummary(
            sessionSummary);
        if (!snapshot.persistedSave.savePath.empty() ||
            snapshot.persistedSave.fileTime != 0 ||
            snapshot.persistedSave.autosaveCompletions != 0)
        {
            RecordDedicatedServerHostedGameRuntimePersistedSave(
                !snapshot.persistedSave.savePath.empty()
                    ? snapshot.persistedSave.savePath
                    : snapshot.context.savePath,
                snapshot.persistedSave.fileTime,
                snapshot.persistedSave.autosaveCompletions);
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
        SetNativeHostedSessionStoppedState(
            &g_nativeHostedSessionState,
            stoppedMs);
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    CaptureNativeDedicatedServerHostedGameSessionState()
    {
        return GetNativeDedicatedServerHostedGameSessionSnapshot();
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    StopNativeDedicatedServerHostedGameSessionAndCaptureFinalState(
        std::uint64_t stoppedMs)
    {
        StopNativeDedicatedServerHostedGameSession(stoppedMs);
        return CaptureNativeDedicatedServerHostedGameSessionState();
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
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
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
        saveStub.worldName = snapshot.context.worldName.empty()
            ? worldName
            : snapshot.context.worldName;
        saveStub.levelId = snapshot.context.worldSaveId.empty()
            ? worldSaveId
            : snapshot.context.worldSaveId;
        saveStub.hostName = snapshot.context.hostName.empty()
            ? hostName
            : snapshot.context.hostName;
        saveStub.bindIp = snapshot.context.bindIp.empty()
            ? bindIp
            : snapshot.context.bindIp;
        saveStub.configuredPort = snapshot.context.configuredPort > 0
            ? snapshot.context.configuredPort
            : configuredPort;
        saveStub.listenerPort = snapshot.context.listenerPort > 0
            ? snapshot.context.listenerPort
            : listenerPort;
        saveStub.onlineGame = snapshot.activation.onlineGame;
        saveStub.privateGame = snapshot.activation.privateGame;
        saveStub.fakeLocalPlayerJoined =
            snapshot.activation.fakeLocalPlayerJoined;
        saveStub.publicSlots = snapshot.activation.publicSlots;
        saveStub.privateSlots = snapshot.activation.privateSlots;
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
            snapshot.worker.pendingWorldActionTicks;
        saveStub.workerPendingAutosaveCommands =
            snapshot.worker.pendingAutosaveCommands;
        saveStub.workerPendingSaveCommands =
            snapshot.worker.pendingSaveCommands;
        saveStub.workerPendingStopCommands =
            snapshot.worker.pendingStopCommands;
        saveStub.workerPendingHaltCommands =
            snapshot.worker.pendingHaltCommands;
        saveStub.workerTickCount = snapshot.worker.workerTickCount;
        saveStub.completedWorkerActions =
            snapshot.worker.completedWorldActions;
        saveStub.processedAutosaveCommands =
            snapshot.worker.processedAutosaveCommands;
        saveStub.processedSaveCommands =
            snapshot.worker.processedSaveCommands;
        saveStub.processedStopCommands =
            snapshot.worker.processedStopCommands;
        saveStub.processedHaltCommands =
            snapshot.worker.processedHaltCommands;
        saveStub.lastQueuedCommandId =
            snapshot.worker.lastQueuedCommandId;
        saveStub.activeCommandId = snapshot.worker.activeCommandId;
        saveStub.activeCommandTicksRemaining =
            snapshot.worker.activeCommandTicksRemaining;
        saveStub.activeCommandKind = snapshot.worker.activeCommandKind;
        saveStub.lastProcessedCommandId =
            snapshot.worker.lastProcessedCommandId;
        saveStub.lastProcessedCommandKind =
            snapshot.worker.lastProcessedCommandKind;
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
        saveStub.initialSaveRequested =
            snapshot.summary.initialSaveRequested;
        saveStub.initialSaveCompleted =
            snapshot.summary.initialSaveCompleted;
        saveStub.initialSaveTimedOut =
            snapshot.summary.initialSaveTimedOut;
        saveStub.sessionCompleted =
            snapshot.summary.sessionCompleted;
        saveStub.requestedAppShutdown =
            snapshot.summary.requestedAppShutdown;
        saveStub.shutdownHaltedGameplay =
            snapshot.summary.shutdownHaltedGameplay;
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
            saveStub.payloadBytes = snapshot.payload.bytes;
            saveStub.payloadChecksum = snapshot.payload.checksum;
            saveStub.saveGeneration = snapshot.saveGeneration;
            saveStub.stateChecksum = snapshot.stateChecksum;
            saveStub.payloadName = snapshot.payload.name;
            saveStub.hostSettings = snapshot.worldConfig.hostSettings;
            saveStub.dedicatedNoLocalHostPlayer =
                snapshot.worldConfig.dedicatedNoLocalHostPlayer;
            saveStub.worldSizeChunks =
                snapshot.worldConfig.worldSizeChunks;
            saveStub.worldHellScale =
                snapshot.worldConfig.worldHellScale;
            saveStub.startupPayloadPresent =
                snapshot.payload.bytes > 0;
            saveStub.startupPayloadValidated =
                snapshot.startup.payloadValidated;
            saveStub.startupThreadIterations =
                snapshot.startup.threadIterations;
            saveStub.startupThreadDurationMs =
                snapshot.startup.threadDurationMs;
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
