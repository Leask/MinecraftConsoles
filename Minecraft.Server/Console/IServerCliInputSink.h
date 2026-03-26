#pragma once

#include <string>
#include <vector>

namespace ServerRuntime
{
    class IServerCliInputSink
    {
    public:
        virtual ~IServerCliInputSink() {}

        virtual void EnqueueCommandLine(const std::string &line) = 0;

        virtual void BuildCompletions(
            const std::string &line,
            std::vector<std::string> *out) const = 0;
    };
}
