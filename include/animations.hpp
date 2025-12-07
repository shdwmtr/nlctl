#pragma once
#include "animation_base.hpp"
#include <cmath>
#include <numbers>
#include <thread>
#include <vector>

namespace led
{

class solid_animation : public animation_base
{
  public:
    using animation_base::animation_base;

    void run() override
    {
        std::vector<color> colors(device_.zone_count(), base_color_);
        device_.set_colors(colors);
    }
};

class breathing_animation : public animation_base
{
    size_t steps_;

  public:
    breathing_animation(hid_device_wrapper& dev, const color& c, std::chrono::milliseconds dur = std::chrono::milliseconds(3000), size_t steps = 500)
        : animation_base(dev, c, dur), steps_(steps)
    {
    }

    void run() override
    {
        auto delay = duration_ / steps_;

        for (size_t frame = 0; frame < steps_; ++frame) {
            double brightness = (std::sin(frame * 2.0 * std::numbers::pi / steps_) + 1.0) / 2.0;
            std::vector<color> colors(device_.zone_count(), base_color_.scaled(brightness));
            device_.set_colors(colors);
            std::this_thread::sleep_for(delay);
        }
    }
};

class wave_animation : public animation_base
{
    size_t frames_;

  public:
    wave_animation(hid_device_wrapper& dev, const color& c, std::chrono::milliseconds dur = std::chrono::milliseconds(2000), size_t frames = 50)
        : animation_base(dev, c, dur), frames_(frames)
    {
    }

    void run() override
    {
        auto delay = duration_ / frames_;
        size_t zones = device_.zone_count();

        for (size_t frame = 0; frame < frames_; ++frame) {
            std::vector<color> colors;
            colors.reserve(zones);

            for (size_t i = 0; i < zones; ++i) {
                double offset = (i + frame) * 2.0 * std::numbers::pi / zones;
                double brightness = (std::sin(offset) + 1.0) / 2.0;
                colors.push_back(base_color_.scaled(brightness));
            }

            device_.set_colors(colors);
            std::this_thread::sleep_for(delay);
        }
    }
};

class rainbow_animation : public animation_base
{
    size_t frames_;

    static color hsv_to_rgb(double h, double s, double v)
    {
        double c = v * s;
        double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
        double m = v - c;

        double r, g, b;
        if (h < 60) {
            r = c;
            g = x;
            b = 0;
        } else if (h < 120) {
            r = x;
            g = c;
            b = 0;
        } else if (h < 180) {
            r = 0;
            g = c;
            b = x;
        } else if (h < 240) {
            r = 0;
            g = x;
            b = c;
        } else if (h < 300) {
            r = x;
            g = 0;
            b = c;
        } else {
            r = c;
            g = 0;
            b = x;
        }

        return { static_cast<uint8_t>((r + m) * 255), static_cast<uint8_t>((g + m) * 255), static_cast<uint8_t>((b + m) * 255) };
    }

  public:
    rainbow_animation(hid_device_wrapper& dev, std::chrono::milliseconds dur = std::chrono::milliseconds(5000), size_t frames = 100)
        : animation_base(dev, color{ 0, 0, 0 }, dur), frames_(frames)
    {
    }

    void run() override
    {
        auto delay = duration_ / frames_;
        size_t zones = device_.zone_count();

        for (size_t frame = 0; frame < frames_; ++frame) {
            std::vector<color> colors;
            colors.reserve(zones);

            for (size_t i = 0; i < zones; ++i) {
                double hue = std::fmod((i * 360.0 / zones) + (frame * 360.0 / frames_), 360.0);
                colors.push_back(hsv_to_rgb(hue, 1.0, 1.0));
            }

            device_.set_colors(colors);
            std::this_thread::sleep_for(delay);
        }
    }
};

} // namespace led