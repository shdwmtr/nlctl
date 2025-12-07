#include "hid_device.hpp"
#include <hidapi.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace led
{

/**
 * Open the device from the vendor ID and product ID.
 */
hid_device_wrapper::hid_device_wrapper(uint16_t vid, uint16_t pid) : device_(hid_open(vid, pid, nullptr)), zone_count_(0)
{
    if (!device_) {
        throw std::runtime_error("Failed to open HID device");
    }
    /** use non blocking IO */
    hid_set_nonblocking(device_, 1);
    zone_count_ = query_zone_count();
}

hid_device_wrapper::~hid_device_wrapper()
{
    if (device_) {
        hid_close(device_);
        hid_exit();
    }
}

void hid_device_wrapper::send_command(uint8_t cmd, const uint8_t* data, size_t data_len, uint8_t* response)
{
    std::array<uint8_t, k_buffer_size> buffer{};
    buffer[1] = cmd;
    buffer[2] = (data_len >> 8) & 0xFF;
    buffer[3] = data_len & 0xFF;

    if (data) {
        std::memcpy(&buffer[4], data, data_len);
    }

    hid_write(device_, buffer.data(), buffer.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int result = hid_read(device_, buffer.data(), k_read_size);
    if (response && result > 0) {
        std::memcpy(response, buffer.data(), k_read_size);
    }
}

void hid_device_wrapper::set_colors(const std::vector<color>& colors)
{
    std::vector<uint8_t> rgb_data(colors.size() * 3);

    for (size_t i = 0; i < colors.size(); ++i) {
        const auto& c = colors[i];
        // Zones 0-19 use GRB, 20+ use RBG
        if (i < 20) {
            rgb_data[i * 3] = c.g;
            rgb_data[i * 3 + 1] = c.r;
            rgb_data[i * 3 + 2] = c.b;
        } else {
            rgb_data[i * 3] = c.r;
            rgb_data[i * 3 + 1] = c.b;
            rgb_data[i * 3 + 2] = c.g;
        }
    }

    write_rgb_data(rgb_data);
}

void hid_device_wrapper::initialize()
{
    uint8_t on = 1;
    uint8_t brightness = 255;
    send_command(0x07, &on, 1);
    send_command(0x09, &brightness, 1);
}

size_t hid_device_wrapper::query_zone_count()
{
    std::array<uint8_t, k_read_size> response{};
    send_command(0x03, nullptr, 0, response.data());
    return response[4];
}

void hid_device_wrapper::write_rgb_data(const std::vector<uint8_t>& rgb_data)
{
    std::array<uint8_t, k_buffer_size> buffer{};
    size_t len = rgb_data.size();

    // First packet
    buffer[1] = 0x02;
    buffer[2] = (len >> 8) & 0xFF;
    buffer[3] = len & 0xFF;
    std::memcpy(&buffer[4], rgb_data.data(), std::min(size_t(60), rgb_data.size()));
    hid_write(device_, buffer.data(), buffer.size());

    // Second packet
    buffer.fill(0);
    if (rgb_data.size() > 60) {
        std::memcpy(&buffer[1], &rgb_data[60], std::min(size_t(64), rgb_data.size() - 60));
    }
    hid_write(device_, buffer.data(), buffer.size());

    // Third packet
    buffer.fill(0);
    if (rgb_data.size() > 124) {
        std::memcpy(&buffer[1], &rgb_data[124], std::min(size_t(38), rgb_data.size() - 124));
    }
    hid_write(device_, buffer.data(), buffer.size());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    hid_read(device_, buffer.data(), k_read_size);
}

} // namespace led
