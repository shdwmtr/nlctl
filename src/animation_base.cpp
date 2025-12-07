#include "animation_base.hpp"

namespace led
{

animation_base::animation_base(hid_device_wrapper& dev, const color& c, std::chrono::milliseconds dur) : device_(dev), base_color_(c), duration_(dur)
{
}

} // namespace led