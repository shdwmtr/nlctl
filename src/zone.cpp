#include <algorithm>
#include <iostream>
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
    std::vector<ZoneColor> zones;
    zones.reserve(bottom_zones + left_zones + top_zones + right_zones);
    int depth = std::min(zone_depth_, std::min(width, height) / 2);

    // Bottom edge (right to left) - ALL zones
    int bottom_zone_width = width / bottom_zones;
    for (int i = bottom_zones - 1; i >= 0; --i) {
        int x_start = i * bottom_zone_width;
        int y_start = height - depth;
        int zone_width = (i == bottom_zones - 1) ? width - x_start : bottom_zone_width;
        zones.push_back(average_region(img_data, width, bpp, x_start, y_start, zone_width, depth));
    }

    // Left edge (bottom to top) - SKIP first zone (bottom-left corner, already in bottom)
    int left_zone_height = height / left_zones;
    for (int i = left_zones - 2; i >= 0; --i) { // Start from left_zones - 2
        int x_start = 0;
        int y_start = i * left_zone_height;
        int zone_height = (i == left_zones - 1) ? height - y_start : left_zone_height;
        zones.push_back(average_region(img_data, width, bpp, x_start, y_start, depth, zone_height));
    }

    // Top edge (left to right) - SKIP first and last zones (corners)
    int top_zone_width = width / top_zones;
    for (int i = 1; i < top_zones - 1; ++i) { // Start from 1, end at top_zones - 1
        int x_start = i * top_zone_width;
        int y_start = 0;
        int zone_width = top_zone_width;
        zones.push_back(average_region(img_data, width, bpp, x_start, y_start, zone_width, depth));
    }

    // Right edge (top to bottom) - SKIP last zone (bottom-right corner, already in bottom)
    int right_zone_height = height / right_zones;
    for (int i = 0; i < right_zones - 1; ++i) { // End at right_zones - 1
        int x_start = width - depth;
        int y_start = i * right_zone_height;
        int zone_height = right_zone_height;
        zones.push_back(average_region(img_data, width, bpp, x_start, y_start, depth, zone_height));
    }

    return zones;
}
