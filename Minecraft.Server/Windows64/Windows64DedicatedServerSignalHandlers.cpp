#include "stdafx.h"

#include "..\Common\DedicatedServerSignalHandlers.h"
#include "..\Common\DedicatedServerSignalState.h"

namespace
{
    static BOOL WINAPI DedicatedServerConsoleCtrlHandler(DWORD ctrlType)
    {
        switch (ctrlType)
        {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            ServerRuntime::RequestDedicatedServerShutdown();
            return TRUE;
        default:
            return FALSE;
        }
    }
}

namespace ServerRuntime
{
    bool InstallDedicatedServerShutdownSignalHandlers()
    {
        return SetConsoleCtrlHandler(
                DedicatedServerConsoleCtrlHandler,
                TRUE) == TRUE;
    }
}
