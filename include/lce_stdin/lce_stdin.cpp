#include "lce_stdin.h"

#if defined(_WINDOWS64) || defined(_WIN32)
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

bool LceStdinIsAvailable()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    return stdinHandle != NULL && stdinHandle != INVALID_HANDLE_VALUE;
#else
    return STDIN_FILENO >= 0;
#endif
}

int LceWaitForStdinReadable(std::uint32_t waitMs)
{
#if defined(_WINDOWS64) || defined(_WIN32)
    HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    if (stdinHandle == NULL || stdinHandle == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    DWORD fileType = GetFileType(stdinHandle);
    if (fileType == FILE_TYPE_PIPE)
    {
        DWORD available = 0;
        if (!PeekNamedPipe(stdinHandle, NULL, 0, NULL, &available, NULL))
        {
            return -1;
        }
        return available > 0 ? 1 : 0;
    }

    if (fileType == FILE_TYPE_CHAR)
    {
        return 1;
    }

    DWORD waitResult = WaitForSingleObject(stdinHandle, waitMs);
    if (waitResult == WAIT_OBJECT_0)
    {
        return 1;
    }
    if (waitResult == WAIT_TIMEOUT)
    {
        return 0;
    }

    return -1;
#else
    pollfd descriptor = {};
    descriptor.fd = STDIN_FILENO;
    descriptor.events = POLLIN;

    const int result = poll(&descriptor, 1, static_cast<int>(waitMs));
    if (result < 0)
    {
        return -1;
    }
    if (result == 0)
    {
        return 0;
    }

    return (descriptor.revents & POLLIN) != 0 ? 1 : -1;
#endif
}

bool LceReadStdinByte(char* outByte)
{
    if (outByte == nullptr)
    {
        return false;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    if (stdinHandle == NULL || stdinHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD bytesRead = 0;
    return ReadFile(stdinHandle, outByte, 1, &bytesRead, NULL) &&
        bytesRead == 1;
#else
    return read(STDIN_FILENO, outByte, 1) == 1;
#endif
}
