#include "Common/NativeDedicatedServerSaveStub.h"
#include "Portability/NativeDedicatedServerHostedGameSession.h"

#include <cstdint>
#include <string>

namespace ServerRuntime
{
    bool IsNativeDedicatedServerHostedGameSessionRunning()
    {
        return false;
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    GetNativeDedicatedServerHostedGameSessionSnapshot()
    {
        return NativeDedicatedServerHostedGameSessionSnapshot();
    }

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionSaveCommand(
        std::uint64_t)
    {
        return 0;
    }

    std::uint64_t EnqueueNativeDedicatedServerHostedGameSessionStopCommand(
        std::uint64_t)
    {
        return 0;
    }

    void ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot(
        std::uint64_t)
    {
    }

    void RefreshNativeDedicatedServerHostedGameSessionWorkerStateAndProject(
        std::uint64_t)
    {
    }

    void ObserveNativeDedicatedServerHostedGameSessionActivityAndProject(
        std::uint64_t,
        std::uint64_t,
        bool,
        std::uint64_t)
    {
    }

    void ObserveNativeDedicatedServerHostedGameSessionContextAndProject(
        const std::string &,
        const std::string &,
        const std::string &,
        const std::string &,
        const std::string &,
        const std::string &,
        int,
        int,
        std::uint64_t,
        std::uint64_t)
    {
    }

    void ObserveNativeDedicatedServerHostedGameSessionPersistedSaveAndProject(
        const std::string &,
        std::uint64_t,
        std::uint64_t,
        std::uint64_t)
    {
    }

    void FinalizeNativeDedicatedServerHostedGameSessionAndProject(
        bool,
        bool,
        bool,
        bool,
        bool,
        bool,
        std::uint64_t,
        bool,
        bool,
        bool,
        std::uint64_t,
        std::uint64_t)
    {
    }

    bool BuildNativeDedicatedServerSaveStubFromSessionSnapshot(
        const std::string &worldName,
        const std::string &worldSaveId,
        const std::string &hostName,
        const std::string &bindIp,
        int configuredPort,
        int listenerPort,
        std::uint64_t,
        std::uint64_t savedAtFileTime,
        NativeDedicatedServerSaveStub *outSaveStub)
    {
        if (outSaveStub == nullptr)
        {
            return false;
        }

        *outSaveStub = NativeDedicatedServerSaveStub();
        outSaveStub->worldName = worldName;
        outSaveStub->levelId = worldSaveId;
        outSaveStub->startupMode = "windows";
        outSaveStub->sessionPhase = "windows-runtime";
        outSaveStub->hostName = hostName;
        outSaveStub->bindIp = bindIp;
        outSaveStub->configuredPort = configuredPort;
        outSaveStub->listenerPort = listenerPort;
        outSaveStub->savedAtFileTime = savedAtFileTime;
        return true;
    }
}
