#pragma once

#include <cstdint>

#include "../ServerProperties.h"
#include "DedicatedServerGameplayLoop.h"
#include "DedicatedServerHostedGameRuntime.h"
#include "DedicatedServerPlatformRuntime.h"
#include "DedicatedServerShutdownPlan.h"
#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    struct DedicatedServerInitialSaveExecutionResult
    {
        bool requested = false;
        bool completed = false;
        bool timedOut = false;
    };

    DedicatedServerInitialSaveExecutionResult
    ExecuteDedicatedServerInitialSave(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        int actionPad);

    struct DedicatedServerWorldLoadExecutionResult
    {
        bool updatedSaveId = false;
        bool savedResolvedSaveId = false;
        bool abortedStartup = false;
        int abortExitCode = 0;
    };

    DedicatedServerWorldLoadExecutionResult ExecuteDedicatedServerWorldLoadPlan(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        const DedicatedServerWorldLoadPlan &worldLoadPlan,
        ServerPropertiesConfig *serverProperties);

    struct DedicatedServerStartupExecutionResult
    {
        DedicatedServerSessionConfig sessionConfig = {};
        DedicatedServerAppSessionApplyResult appSession = {};
        DedicatedServerWorldBootstrapPlan worldBootstrapPlan = {};
        DedicatedServerWorldLoadExecutionResult worldLoadExecution = {};
        DedicatedServerHostedGamePlan hostedGamePlan = {};
        DedicatedServerHostedGameStartupExecutionResult hostedGameStartup = {};
        bool abortedStartup = false;
        int abortExitCode = 0;
    };

    struct DedicatedServerShutdownExecutionResult
    {
        DedicatedServerShutdownPlan plan = {};
        bool haltedGameplay = false;
    };

    template <typename TInitData>
    DedicatedServerStartupExecutionResult ExecuteDedicatedServerStartup(
        const DedicatedServerConfig &config,
        ServerPropertiesConfig *serverProperties,
        const WorldBootstrapResult &worldBootstrap,
        std::int64_t generatedSeed,
        DedicatedServerHostedGameThreadProc *threadProc,
        TInitData *initData)
    {
        DedicatedServerStartupExecutionResult result = {};
        if (serverProperties == nullptr)
        {
            result.abortedStartup = true;
            result.abortExitCode = 1;
            return result;
        }

        result.sessionConfig = BuildDedicatedServerSessionConfig(
            config,
            *serverProperties);
        result.appSession = ApplyDedicatedServerAppSessionPlan(
            BuildDedicatedServerAppSessionPlan(result.sessionConfig));
        result.worldBootstrapPlan = BuildDedicatedServerWorldBootstrapPlan(
            *serverProperties,
            worldBootstrap);
        result.worldLoadExecution = ExecuteDedicatedServerWorldLoadPlan(
            result.worldBootstrapPlan,
            BuildDedicatedServerWorldLoadPlan(result.worldBootstrapPlan),
            serverProperties);
        if (result.worldLoadExecution.abortedStartup)
        {
            result.abortedStartup = true;
            result.abortExitCode = result.worldLoadExecution.abortExitCode;
            return result;
        }

        result.hostedGamePlan = BuildDedicatedServerHostedGamePlan(
            result.sessionConfig,
            worldBootstrap.saveData,
            generatedSeed);
        result.hostedGameStartup = ExecuteDedicatedServerHostedGameStartup(
            result.hostedGamePlan,
            threadProc,
            initData);
        if (result.hostedGameStartup.startupPlan.shouldAbortStartup)
        {
            result.abortedStartup = true;
            result.abortExitCode =
                result.hostedGameStartup.startupPlan.abortExitCode;
        }

        return result;
    }

    struct DedicatedServerSessionExecutionResult
    {
        DedicatedServerInitialSaveExecutionResult initialSave = {};
        DedicatedServerAutosaveState autosaveState = {};
        DedicatedServerGameplayLoopRunResult gameplayLoop = {};
        DedicatedServerShutdownExecutionResult shutdown = {};
    };

    struct DedicatedServerRunExecutionResult
    {
        DedicatedServerStartupExecutionResult startup = {};
        DedicatedServerSessionExecutionResult session = {};
        bool completedSession = false;
        bool abortedStartup = false;
        int abortExitCode = 0;
    };

    DedicatedServerSessionExecutionResult ExecuteDedicatedServerSession(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        const ServerPropertiesConfig &serverProperties,
        int actionPad,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext,
        std::uint64_t initialTickMs,
        unsigned int sleepMs);

    template <typename TInitData>
    DedicatedServerRunExecutionResult ExecuteDedicatedServerRun(
        const DedicatedServerConfig &config,
        ServerPropertiesConfig *serverProperties,
        const WorldBootstrapResult &worldBootstrap,
        int actionPad,
        std::int64_t generatedSeed,
        DedicatedServerHostedGameThreadProc *threadProc,
        TInitData *initData,
        DedicatedServerGameplayLoopPollProc *afterStartupProc,
        void *afterStartupContext,
        DedicatedServerGameplayLoopPollProc *beforeSessionProc,
        void *beforeSessionContext,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext,
        DedicatedServerGameplayLoopPollProc *afterSessionProc,
        void *afterSessionContext,
        std::uint64_t initialTickMs,
        unsigned int sleepMs)
    {
        DedicatedServerRunExecutionResult result = {};
        result.startup = ExecuteDedicatedServerStartup(
            config,
            serverProperties,
            worldBootstrap,
            generatedSeed,
            threadProc,
            initData);
        if (result.startup.abortedStartup)
        {
            result.abortedStartup = true;
            result.abortExitCode = result.startup.abortExitCode;
            return result;
        }

        if (afterStartupProc != nullptr)
        {
            afterStartupProc(afterStartupContext);
        }
        if (beforeSessionProc != nullptr)
        {
            beforeSessionProc(beforeSessionContext);
        }
        result.session = ExecuteDedicatedServerSession(
            result.startup.worldBootstrapPlan,
            *serverProperties,
            actionPad,
            pollProc,
            pollContext,
            initialTickMs,
            sleepMs);
        if (afterSessionProc != nullptr)
        {
            afterSessionProc(afterSessionContext);
        }
        result.completedSession = true;
        return result;
    }

    DedicatedServerShutdownExecutionResult ExecuteDedicatedServerShutdown();
}
