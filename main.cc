#include <hidapi.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <thread>
#include <chrono>

struct RGB
{
    unsigned char r, g, b;
};

RGB hsv_to_rgb(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;

    int sector = (int)(h / 60.0f) % 6;

    float r6[6] = { c, x, 0, 0, x, c };
    float g6[6] = { x, c, c, x, 0, 0 };
    float b6[6] = { 0, 0, x, c, c, x };

    float r = r6[sector];
    float g = g6[sector];
    float b = b6[sector];

    return { (unsigned char)((r + m) * 255), (unsigned char)((g + m) * 255), (unsigned char)((b + m) * 255) };
}

void set_colors(hid_device* dev, int num_zones, RGB* colors)
{
    int total_rgb_bytes = num_zones * 3;
    int zones_first = 20;

    // First packet
    unsigned char packet1[65] = { 0 };
    packet1[1] = 0x02;
    packet1[2] = (total_rgb_bytes >> 8) & 0xFF;
    packet1[3] = total_rgb_bytes & 0xFF;

    for (int i = 0; i < zones_first; i++) {
        packet1[4 + i * 3] = colors[i].r;
        packet1[4 + i * 3 + 1] = colors[i].g;
        packet1[4 + i * 3 + 2] = colors[i].b;
    }

    hid_write(dev, packet1, 65);

    // Continuation packets
    int zones_sent = zones_first;
    while (zones_sent < num_zones) {
        int zones_this_packet = std::min(21, num_zones - zones_sent);

        unsigned char packet[65] = { 0 };
        for (int i = 0; i < zones_this_packet; i++) {
            packet[1 + i * 3] = colors[zones_sent + i].r;
            packet[1 + i * 3 + 1] = colors[zones_sent + i].g;
            packet[1 + i * 3 + 2] = colors[zones_sent + i].b;
        }

        hid_write(dev, packet, 65);
        zones_sent += zones_this_packet;
    }

    unsigned char response[256];
    hid_read_timeout(dev, response, sizeof(response), 100);
}

int main()
{
    if (hid_init() < 0) return 1;

    hid_device* dev = hid_open(0x37FA, 0x8202, nullptr);
    if (!dev) {
        std::cerr << "Failed to open device\n";
        hid_exit();
        return 1;
    }
    std::cout << "Device opened\n";

    unsigned char get_len[65] = { 0 };
    get_len[1] = 0x03;
    hid_write(dev, get_len, 65);

    unsigned char response[256] = { 0 };
    hid_read_timeout(dev, response, sizeof(response), 1000);
    int num_zones = response[4];
    std::cout << "Number of zones: " << num_zones << "\n";

    unsigned char set_on[65] = { 0 };
    set_on[1] = 0x07;
    set_on[2] = 0x00;
    set_on[3] = 0x01;
    set_on[4] = 0x01;
    hid_write(dev, set_on, 65);
    hid_read_timeout(dev, response, sizeof(response), 1000);

    unsigned char set_brightness[65] = { 0 };
    set_brightness[1] = 0x09;
    set_brightness[2] = 0x00;
    set_brightness[3] = 0x01;
    set_brightness[4] = 0xFF;
    hid_write(dev, set_brightness, 65);
    hid_read_timeout(dev, response, sizeof(response), 1000);

    std::cout << "Starting rainbow cycle (Ctrl+C to stop)...\n";

    RGB colors[54];
    float hue_offset = 0;

    while (true) {
        for (int i = 0; i < num_zones; i++) {
            float hue = fmod(hue_offset + (i * 360.0 / num_zones), 360.0);
            colors[i] = hsv_to_rgb(hue, 1.0, 1.0);
        }

        set_colors(dev, num_zones, colors);

        hue_offset += 5.0;
        if (hue_offset >= 360) hue_offset -= 360;

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    hid_close(dev);
    hid_exit();
    return 0;
}