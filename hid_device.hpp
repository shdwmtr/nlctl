#pragma once
#include "color.hpp"
#include <array>
#include <cstdint>
#include <vector>

struct hid_device_;
typedef struct hid_device_ hid_device;

namespace led
{

constexpr uint16_t k_vendor_id = 0x37FA;
constexpr uint16_t k_product_id = 0x8202;
constexpr size_t k_buffer_size = 65;
constexpr size_t k_read_size = 64;

class hid_device_wrapper
{
    hid_device* device_;
    size_t zone_count_;

    size_t query_zone_count();
    void write_rgb_data(const std::vector<uint8_t>& rgb_data);

  public:
    explicit hid_device_wrapper(uint16_t vid = k_vendor_id, uint16_t pid = k_product_id);
    ~hid_device_wrapper();

    hid_device_wrapper(const hid_device_wrapper&) = delete;
    hid_device_wrapper& operator=(const hid_device_wrapper&) = delete;

    size_t zone_count() const
    {
        return zone_count_;
    }

    void send_command(uint8_t cmd, const uint8_t* data, size_t data_len, uint8_t* response = nullptr);
    void set_colors(const std::vector<color>& colors);
    void initialize();
};

} // namespace led