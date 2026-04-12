#include "lce_stdin.h"

#include <poll.h>
#include <unistd.h>

bool LceStdinIsAvailable()
{
    return STDIN_FILENO >= 0;
}

int LceWaitForStdinReadable(std::uint32_t waitMs)
{
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
}

bool LceReadStdinByte(char* outByte)
{
    if (outByte == nullptr)
    {
        return false;
    }

    return read(STDIN_FILENO, outByte, 1) == 1;
}
