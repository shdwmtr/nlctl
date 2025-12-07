#include "color.hpp"

namespace led
{

color::color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue)
{
}

color color::scaled(double brightness) const
{
    return { static_cast<uint8_t>(r * brightness), static_cast<uint8_t>(g * brightness), static_cast<uint8_t>(b * brightness) };
}

} // namespace led