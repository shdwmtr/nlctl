#pragma once
#include <cstdint>

namespace led
{

struct color
{
    uint8_t r, g, b;

    color(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0);
    color scaled(double brightness) const;
};

} // namespace led