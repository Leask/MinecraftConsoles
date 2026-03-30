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
        std::uint64_t observedAutosaveCompletions = 0;
        std::uint64_t saveGeneration = 0;
        std::uint64_t stateChecksum = 0;
    };

    void ResetNativeDedicatedServerHostedGameSessionState();

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData,
        bool startupPayloadValidated);

    void TickNativeDedicatedServerHostedGameSession();

    void ObserveNativeDedicatedServerHostedGameSessionAutosaves(
        std::uint64_t autosaveCompletions);

    void StopNativeDedicatedServerHostedGameSession();

    bool IsNativeDedicatedServerHostedGameSessionRunning();

    std::uint64_t GetNativeDedicatedServerHostedGameSessionThreadTicks();

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot();

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode = nullptr);
}
