#pragma once

#include <cstdio>
#include <cstdint>
#include <string>

#include "../Access/Access.h"
#include "DedicatedServerOptions.h"
#include "DedicatedServerRuntime.h"
#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    enum EDedicatedServerBootstrapStatus
    {
        eDedicatedServerBootstrap_Ready = 0,
        eDedicatedServerBootstrap_ShowHelp = 1,
        eDedicatedServerBootstrap_Failed = 2
    };

    struct DedicatedServerBootstrapContext
    {
        DedicatedServerConfig config = {};
        ServerPropertiesConfig serverProperties = {};
        DedicatedServerWorldTarget worldTarget = {};
        DedicatedServerRuntimeState runtimeState = {};
        std::string storageRoot;
        std::uint64_t autosaveIntervalMs = 0;
        bool accessInitialized = false;
    };

    void ShutdownDedicatedServerBootstrapEnvironment(
        DedicatedServerBootstrapContext *context);

    class DedicatedServerBootstrapEnvironmentGuard
    {
    public:
        DedicatedServerBootstrapEnvironmentGuard()
            : m_context(nullptr)
        {
        }

        void Activate(DedicatedServerBootstrapContext *context)
        {
            m_context = context;
        }

        void Release()
        {
            m_context = nullptr;
        }

        ~DedicatedServerBootstrapEnvironmentGuard()
        {
            ShutdownDedicatedServerBootstrapEnvironment(m_context);
        }

    private:
        DedicatedServerBootstrapContext *m_context;
    };

    EDedicatedServerBootstrapStatus PrepareDedicatedServerBootstrapContext(
        int argc,
        char **argv,
        DedicatedServerBootstrapContext *context,
        std::string *outError);

    bool InitializeDedicatedServerBootstrapEnvironment(
        DedicatedServerBootstrapContext *context,
        std::string *outError);

    void ShutdownDedicatedServerBootstrapEnvironment(
        DedicatedServerBootstrapContext *context);

    void PrintDedicatedServerUsage(FILE *stream);
    void LogDedicatedServerUsage();

    void LogDedicatedServerBootstrapSummary(
        const char *label,
        const DedicatedServerBootstrapContext &context);
}
