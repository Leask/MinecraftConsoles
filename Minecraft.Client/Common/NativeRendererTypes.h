#pragma once

#include <cstdint>

struct NativeRendererViewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct NativeRendererRect
{
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

struct NativeRendererSize
{
    std::int32_t width = 0;
    std::int32_t height = 0;
};
