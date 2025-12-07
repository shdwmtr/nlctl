#include "animations.hpp"
#include "hid_device.hpp"
#include <cstring>
#include <iostream>
#include <memory>
#include <iostream>
#include <cstring>

enum class mode
{
    solid,
    breathing,
    wave,
    rainbow,
    screen_zones
};

int main(int argc, char* argv[])
{
    try {
        int bottom_zones = 10, left_zones = 10, top_zones = 10, right_zones = 10;

        led::color clr{ 255, 255, 255 };
        mode run_mode = mode::solid;

        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--color") == 0 && i + 1 < argc) {
                int r, g, b;
                if (std::sscanf(argv[++i], "%d,%d,%d", &r, &g, &b) != 3) {
                    std::cerr << "Invalid color format. Use: --color r,g,b\n";
                    return 1;
                }
                clr = { static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b) };
            } else if (std::strcmp(argv[i], "--zones") == 0 && i + 1 < argc) {
                if (std::sscanf(argv[++i], "%d,%d,%d,%d", &bottom_zones, &left_zones, &top_zones, &right_zones) != 4) {
                    std::cerr << "Invalid zones format. Use: --zones bottom,left,top,right\n";
                    return 1;
                }
            } else if (std::strcmp(argv[i], "--breathing") == 0) {
                run_mode = mode::breathing;
            } else if (std::strcmp(argv[i], "--wave") == 0) {
                run_mode = mode::wave;
            } else if (std::strcmp(argv[i], "--rainbow") == 0) {
                run_mode = mode::rainbow;
            } else if (std::strcmp(argv[i], "--screen-zones") == 0) {
                run_mode = mode::screen_zones;
            }
        }

        led::hid_device_wrapper device;
        std::cout << "Number of LED's in strip: " << device.zone_count() << '\n';

        device.initialize();

        std::unique_ptr<led::animation_base> anim;

        switch (run_mode) {
            case mode::breathing:
                std::cout << "Running breathing animation with color (" << static_cast<int>(clr.r) << "," << static_cast<int>(clr.g) << "," << static_cast<int>(clr.b)
                          << ") (Ctrl+C to stop)...\n";
                anim = std::make_unique<led::breathing_animation>(device, clr);
                while (true)
                    anim->run();
                break;

            case mode::wave:
                std::cout << "Running wave animation with color (" << static_cast<int>(clr.r) << "," << static_cast<int>(clr.g) << "," << static_cast<int>(clr.b)
                          << ") (Ctrl+C to stop)...\n";
                anim = std::make_unique<led::wave_animation>(device, clr);
                while (true)
                    anim->run();
                break;

            case mode::rainbow:
                std::cout << "Running rainbow animation (Ctrl+C to stop)...\n";
                anim = std::make_unique<led::rainbow_animation>(device);
                while (true)
                    anim->run();
                break;

            case mode::solid:
                std::cout << "Setting solid color (" << static_cast<int>(clr.r) << "," << static_cast<int>(clr.g) << "," << static_cast<int>(clr.b) << ")\n";
                anim = std::make_unique<led::solid_animation>(device, clr, std::chrono::milliseconds(0));
                anim->run();
                break;

            case mode::screen_zones:
                std::cout << "Running screen zone animation with zones (B:" << bottom_zones << " L:" << left_zones << " T:" << top_zones << " R:" << right_zones
                          << ") (Ctrl+C to stop)...\n";
                anim = std::make_unique<led::screen_zone_animation>(device, bottom_zones, left_zones, top_zones, right_zones,
                                                                    0.9f, // 50% screen capture
                                                                    50,   // 50px zone depth
                                                                    60);  // 30 fps
                while (true)
                    anim->run();
                break;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
