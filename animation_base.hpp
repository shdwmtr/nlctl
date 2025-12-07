#pragma once
#include "color.hpp"
#include "hid_device.hpp"
#include <chrono>

namespace led
{

class animation_base
{
  protected:
    hid_device_wrapper& device_;
    color base_color_;
    std::chrono::milliseconds duration_;

  public:
    animation_base(hid_device_wrapper& dev, const color& c, std::chrono::milliseconds dur);
    virtual ~animation_base() = default;
    virtual void run() = 0;
};

} // namespace led