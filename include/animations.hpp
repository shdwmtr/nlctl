#pragma once
#include "animation_base.hpp"
#include "x11.hpp"
#include "zone.hpp"
#include <cmath>
#include <iostream>
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

class screen_zone_animation : public animation_base
{
    size_t bottom_zones_;
    size_t left_zones_;
    size_t top_zones_;
    size_t right_zones_;
    float capture_percent_;
    int zone_depth_;
    size_t fps_;

  public:
    screen_zone_animation(hid_device_wrapper& dev, size_t bottom_zones, size_t left_zones, size_t top_zones, size_t right_zones, float capture_percent = 0.5f, int zone_depth = 50,
                          size_t fps = 30)
        : animation_base(dev, color{ 0, 0, 0 }, std::chrono::milliseconds(0)), bottom_zones_(bottom_zones), left_zones_(left_zones), top_zones_(top_zones),
          right_zones_(right_zones), capture_percent_(capture_percent), zone_depth_(zone_depth), fps_(fps)
    {
    }

    void run() override
    {
        auto frame_start = std::chrono::high_resolution_clock::now();

        ScreenCapture cap;
        ZoneAnalyzer analyzer(zone_depth_);
        auto delay = std::chrono::milliseconds(1000 / fps_);

        if (!cap.capture(capture_percent_)) {
            return;
        }

        auto zones = analyzer.analyze(cap.data(), cap.width(), cap.height(), cap.bytes_per_pixel(), bottom_zones_, left_zones_, top_zones_, right_zones_);

        // Verify zone count matches LED count
        size_t total_zones = bottom_zones_ + left_zones_ + top_zones_ + right_zones_;
        if (zones.size() != device_.zone_count()) {
            std::cerr << "Warning: Zone count (" << zones.size() << ") doesn't match LED count (" << device_.zone_count() << ")\n";
        }

        std::vector<color> colors;
        colors.reserve(zones.size());

        for (const auto& zone : zones) {
            colors.push_back({ static_cast<uint8_t>(zone.r * 255), static_cast<uint8_t>(zone.g * 255), static_cast<uint8_t>(zone.b * 255) });
        }

        device_.set_colors(colors);

        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);
        std::cout << "Frame time: " << frame_time.count() << "ms\n";

        std::this_thread::sleep_for(delay);
    }
};

} // namespace led
