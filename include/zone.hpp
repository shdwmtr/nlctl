#pragma once
#include <cstdint>
#include <vector>

struct ZoneColor
{
    float r, g, b;
};

class ZoneAnalyzer
{
  public:
    ZoneAnalyzer(int zone_depth = 50);

    std::vector<ZoneColor> analyze(uint8_t* img_data, int width, int height, int bpp, int bottom_zones, int left_zones, int top_zones, int right_zones);

    void set_zone_depth(int depth)
    {
        zone_depth_ = depth;
    }

  private:
    int zone_depth_;

    ZoneColor average_region(uint8_t* data, int img_width, int bpp, int x_start, int y_start, int region_width, int region_height);
};
