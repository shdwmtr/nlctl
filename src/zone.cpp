#include <algorithm>
#include "zone.hpp"

ZoneAnalyzer::ZoneAnalyzer(int zone_depth) : zone_depth_(zone_depth)
{
}

ZoneColor ZoneAnalyzer::average_region(uint8_t* data, int img_width, int bpp, int x_start, int y_start, int region_width, int region_height)
{
    uint64_t r_sum = 0, g_sum = 0, b_sum = 0;
    int pixel_count = 0;

    for (int y = y_start; y < y_start + region_height; ++y) {
        for (int x = x_start; x < x_start + region_width; ++x) {
            int idx = (y * img_width + x) * bpp;

            // Assuming BGRA format (common for X11)
            b_sum += data[idx];
            g_sum += data[idx + 1];
            r_sum += data[idx + 2];
            pixel_count++;
        }
    }

    return { static_cast<float>(r_sum) / pixel_count / 255.0f, static_cast<float>(g_sum) / pixel_count / 255.0f, static_cast<float>(b_sum) / pixel_count / 255.0f };
}
std::vector<ZoneColor> ZoneAnalyzer::analyze(uint8_t* img_data, int width, int height, int bpp, int bottom_zones, int left_zones, int top_zones, int right_zones)
{
    int total = bottom_zones + left_zones - 1 + top_zones - 2 + right_zones - 1;
    std::vector<ZoneColor> zones(total);

    int depth = std::min(zone_depth_, std::min(width, height) / 2);

#pragma omp parallel
    {
        int idx = 0;

#pragma omp for schedule(static)
        for (int i = 0; i < bottom_zones; ++i) {
            int rev_i = bottom_zones - 1 - i;
            int x = rev_i * (width / bottom_zones);
            int w = (rev_i == bottom_zones - 1) ? width - x : width / bottom_zones;
            zones[i] = average_region(img_data, width, bpp, x, height - depth, w, depth);
        }

#pragma omp for schedule(static)
        for (int i = 0; i < left_zones - 1; ++i) {
            int rev_i = left_zones - 2 - i;
            int y = rev_i * (height / left_zones);
            int h = (rev_i == left_zones - 1) ? height - y : height / left_zones;
            zones[bottom_zones + i] = average_region(img_data, width, bpp, 0, y, depth, h);
        }

#pragma omp for schedule(static)
        for (int i = 0; i < top_zones - 2; ++i) {
            int actual_i = i + 1;
            zones[bottom_zones + left_zones - 1 + i] = average_region(img_data, width, bpp, actual_i * (width / top_zones), 0, width / top_zones, depth);
        }

#pragma omp for schedule(static)
        for (int i = 0; i < right_zones - 1; ++i) {
            zones[bottom_zones + left_zones - 1 + top_zones - 2 + i] = average_region(img_data, width, bpp, width - depth, i * (height / right_zones), depth, height / right_zones);
        }
    }
    return zones;
}
