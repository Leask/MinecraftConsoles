#include "DedicatedServerSocketBootstrap.h"

namespace ServerRuntime
{
    bool StartDedicatedServerSocketBootstrap(
        const DedicatedServerBootstrapContext &context,
        DedicatedServerSocketBootstrapState *state,
        std::string *outError)
    {
        if (state == nullptr)
        {
            if (outError != nullptr)
            {
                *outError = "missing dedicated server socket state";
            }
            return false;
        }

        if (outError != nullptr)
        {
            outError->clear();
        }

        *state = DedicatedServerSocketBootstrapState{};
        state->listener = LceNetOpenTcpSocket();
        if (state->listener == LCE_INVALID_SOCKET)
        {
            if (outError != nullptr)
            {
                *outError = "failed to open dedicated server listener socket";
            }
            return false;
        }

        if (!LceNetSetSocketReuseAddress(state->listener, true))
        {
            if (outError != nullptr)
            {
                *outError = "failed to enable reuseaddr on dedicated listener";
            }
            StopDedicatedServerSocketBootstrap(state);
            return false;
        }

        if (!LceNetBindIpv4(
                state->listener,
                context.runtimeState.bindIp.c_str(),
                context.runtimeState.multiplayerPort))
        {
            if (outError != nullptr)
            {
                *outError = "failed to bind dedicated listener socket";
            }
            StopDedicatedServerSocketBootstrap(state);
            return false;
        }

        if (!LceNetListen(state->listener, 16))
        {
            if (outError != nullptr)
            {
                *outError = "failed to listen on dedicated listener socket";
            }
            StopDedicatedServerSocketBootstrap(state);
            return false;
        }

        state->boundPort = LceNetGetBoundPort(state->listener);
        if (state->boundPort <= 0)
        {
            if (outError != nullptr)
            {
                *outError =
                    "failed to resolve dedicated listener bound port";
            }
            StopDedicatedServerSocketBootstrap(state);
            return false;
        }

        return true;
    }

    void StopDedicatedServerSocketBootstrap(
        DedicatedServerSocketBootstrapState *state)
    {
        if (state == nullptr)
        {
            return;
        }

        if (state->listener != LCE_INVALID_SOCKET)
        {
            LceNetCloseSocket(state->listener);
            state->listener = LCE_INVALID_SOCKET;
        }
        state->boundPort = -1;
    }
}
