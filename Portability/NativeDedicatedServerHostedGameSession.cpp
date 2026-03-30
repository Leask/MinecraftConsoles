#include "NativeDedicatedServerHostedGameSession.h"

#include <mutex>
#include <string>

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"

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
            std::uint64_t baseProcessedSaveCommands = 0;
            std::uint64_t baseProcessedStopCommands = 0;
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
                state->snapshot.workerPendingSaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerPendingStopCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.workerTickCount);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.completedWorkerActions);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.processedSaveCommands);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.processedStopCommands);
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
                state->snapshot.lastPersistedFileTime);
            checksum = MixNativeHostedSessionHash(
                checksum,
                state->snapshot.lastPersistedAutosaveCompletions);
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
    }

    void ResetNativeDedicatedServerHostedGameSessionCoreState()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState = NativeDedicatedServerHostedGameSessionState{};
    }

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData,
        bool startupPayloadValidated)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState = NativeDedicatedServerHostedGameSessionState{};
        g_nativeHostedSessionState.snapshot.active = startupPayloadValidated;
        g_nativeHostedSessionState.snapshot.loadedFromSave =
            initData.saveData != nullptr;
        g_nativeHostedSessionState.snapshot.payloadValidated =
            startupPayloadValidated;
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
            const char *payloadBytes =
                static_cast<const char *>(initData.saveData->data);
            const std::string payloadText(
                payloadBytes,
                payloadBytes + initData.saveData->fileSize);
            NativeDedicatedServerSaveStub saveStub = {};
            if (ParseNativeDedicatedServerSaveStubText(payloadText, &saveStub))
            {
                g_nativeHostedSessionState.baseSaveGeneration =
                    saveStub.saveGeneration;
                g_nativeHostedSessionState.baseStateChecksum =
                    saveStub.stateChecksum;
                g_nativeHostedSessionState.baseWorkerTickCount =
                    saveStub.workerTickCount;
                g_nativeHostedSessionState.baseCompletedWorkerActions =
                    saveStub.completedWorkerActions;
                g_nativeHostedSessionState.baseProcessedSaveCommands =
                    saveStub.processedSaveCommands;
                g_nativeHostedSessionState.baseProcessedStopCommands =
                    saveStub.processedStopCommands;
                g_nativeHostedSessionState.baseGameplayLoopIterations =
                    saveStub.gameplayLoopIterations;
                g_nativeHostedSessionState.snapshot.sessionTicks =
                    saveStub.hostedThreadTicks;
                g_nativeHostedSessionState.snapshot.workerTickCount =
                    saveStub.workerTickCount;
                g_nativeHostedSessionState.snapshot.completedWorkerActions =
                    saveStub.completedWorkerActions;
                g_nativeHostedSessionState.snapshot.processedSaveCommands =
                    saveStub.processedSaveCommands;
                g_nativeHostedSessionState.snapshot.processedStopCommands =
                    saveStub.processedStopCommands;
                g_nativeHostedSessionState.snapshot.gameplayLoopIterations =
                    saveStub.gameplayLoopIterations;
                g_nativeHostedSessionState.snapshot.lastPersistedFileTime =
                    saveStub.savedAtFileTime;
                g_nativeHostedSessionState.snapshot
                    .lastPersistedAutosaveCompletions =
                    saveStub.autosaveCompletions;
            }
        }

        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
        return startupPayloadValidated;
    }

    void ObserveNativeDedicatedServerHostedGameSessionStartupResult(
        int startupResult,
        bool threadInvoked)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.startupResult = startupResult;
        g_nativeHostedSessionState.snapshot.threadInvoked = threadInvoked;
        g_nativeHostedSessionState.snapshot.runtimePhase =
            startupResult == 0
                ? eDedicatedServerHostedGameRuntimePhase_Startup
                : eDedicatedServerHostedGameRuntimePhase_Failed;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void TickNativeDedicatedServerHostedGameSession()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        if (!g_nativeHostedSessionState.snapshot.active)
        {
            return;
        }

        ++g_nativeHostedSessionState.snapshot.sessionTicks;
        ++g_nativeHostedSessionState.snapshot.gameplayLoopIterations;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
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

    void ObserveNativeDedicatedServerHostedGameSessionContext(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &savePath,
        const std::string &storageRoot,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort)
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
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
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
        if (appShutdownRequested || gameplayHalted)
        {
            g_nativeHostedSessionState.snapshot.runtimePhase =
                eDedicatedServerHostedGameRuntimePhase_ShutdownRequested;
        }
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
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

    void ObserveNativeDedicatedServerHostedGameSessionPhase(
        int runtimePhase)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.runtimePhase = runtimePhase;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
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
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
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

    void ObserveNativeDedicatedServerHostedGameSessionWorkerState(
        std::uint64_t pendingWorldActionTicks,
        std::uint64_t pendingSaveCommands,
        std::uint64_t pendingStopCommands,
        std::uint64_t workerTickCount,
        std::uint64_t completedWorkerActions,
        std::uint64_t processedSaveCommands,
        std::uint64_t processedStopCommands)
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.workerPendingWorldActionTicks =
            pendingWorldActionTicks;
        g_nativeHostedSessionState.snapshot.workerPendingSaveCommands =
            pendingSaveCommands;
        g_nativeHostedSessionState.snapshot.workerPendingStopCommands =
            pendingStopCommands;
        g_nativeHostedSessionState.snapshot.workerTickCount =
            g_nativeHostedSessionState.baseWorkerTickCount +
            workerTickCount;
        g_nativeHostedSessionState.snapshot.completedWorkerActions =
            g_nativeHostedSessionState.baseCompletedWorkerActions +
            completedWorkerActions;
        g_nativeHostedSessionState.snapshot.processedSaveCommands =
            g_nativeHostedSessionState.baseProcessedSaveCommands +
            processedSaveCommands;
        g_nativeHostedSessionState.snapshot.processedStopCommands =
            g_nativeHostedSessionState.baseProcessedStopCommands +
            processedStopCommands;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    void StopNativeDedicatedServerHostedGameSession()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        g_nativeHostedSessionState.snapshot.active = false;
        g_nativeHostedSessionState.snapshot.runtimePhase =
            eDedicatedServerHostedGameRuntimePhase_Stopped;
        RefreshNativeHostedSessionStateChecksum(
            &g_nativeHostedSessionState);
    }

    bool IsNativeDedicatedServerHostedGameSessionRunning()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        return g_nativeHostedSessionState.snapshot.active;
    }

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        return g_nativeHostedSessionState.snapshot.sessionTicks;
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot()
    {
        std::lock_guard<std::mutex> lock(g_nativeHostedSessionMutex);
        return g_nativeHostedSessionState.snapshot;
    }
}
