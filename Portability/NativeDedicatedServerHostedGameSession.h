#pragma once

#include <cstdint>

#include <lce_win32/lce_win32.h>

#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameSessionSnapshot
    {
        bool active = false;
        bool loadedFromSave = false;
        bool payloadValidated = false;
        std::uint64_t payloadChecksum = 0;
        std::uint64_t sessionTicks = 0;
        std::uint64_t acceptedConnections = 0;
        std::uint64_t remoteCommands = 0;
        std::uint64_t observedAutosaveCompletions = 0;
        std::uint64_t gameplayLoopIterations = 0;
        std::uint64_t saveGeneration = 0;
        std::uint64_t stateChecksum = 0;
        std::uint64_t lastPersistedFileTime = 0;
        std::uint64_t lastPersistedAutosaveCompletions = 0;
        std::uint64_t startupThreadIterations = 0;
        std::uint64_t startupThreadDurationMs = 0;
        std::uint64_t hostedThreadTicks = 0;
        bool worldActionIdle = true;
        bool appShutdownRequested = false;
        bool gameplayHalted = false;
        bool stopSignalValid = false;
        bool hostedThreadActive = false;
        bool initialSaveRequested = false;
        bool initialSaveCompleted = false;
        bool initialSaveTimedOut = false;
        bool sessionCompleted = false;
        bool requestedAppShutdown = false;
        bool shutdownHaltedGameplay = false;
    };

    void ResetNativeDedicatedServerHostedGameSessionState();

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData,
        bool startupPayloadValidated);

    void TickNativeDedicatedServerHostedGameSession();

    void ObserveNativeDedicatedServerHostedGameSessionAutosaves(
        std::uint64_t autosaveCompletions);

    void ObserveNativeDedicatedServerHostedGameSessionActivity(
        std::uint64_t acceptedConnections,
        std::uint64_t remoteCommands,
        bool worldActionIdle);

    void ObserveNativeDedicatedServerHostedGameSessionRuntimeState(
        std::uint64_t gameplayLoopIterations,
        bool appShutdownRequested,
        bool gameplayHalted,
        bool stopSignalValid);

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSave(
        std::uint64_t savedAtFileTime,
        std::uint64_t autosaveCompletions);

    void ObserveNativeDedicatedServerHostedGameSessionSummary(
        bool initialSaveRequested,
        bool initialSaveCompleted,
        bool initialSaveTimedOut,
        bool sessionCompleted,
        bool requestedAppShutdown,
        bool shutdownHaltedGameplay);

    void ObserveNativeDedicatedServerHostedGameSessionThreadState(
        bool hostedThreadActive,
        std::uint64_t hostedThreadTicks);

    void ObserveNativeDedicatedServerHostedGameSessionStartupTelemetry(
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs);

    void StopNativeDedicatedServerHostedGameSession();

    bool IsNativeDedicatedServerHostedGameSessionRunning();

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);
}
