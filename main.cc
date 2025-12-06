#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif
#include <hidapi.h>
#include <math.h>

#define VID 0x37FA
#define PID 0x8202
#define PI 3.14159265

int send_command(hid_device* dev, unsigned char cmd, unsigned char* data, int data_len, unsigned char* response)
{
    unsigned char buf[65] = { 0 };
    buf[0] = 0x00;
    buf[1] = cmd;
    buf[2] = (data_len >> 8) & 0xFF;
    buf[3] = data_len & 0xFF;
    if (data) memcpy(&buf[4], data, data_len);
    hid_write(dev, buf, 65);
    SLEEP_MS(100);
    int res = hid_read(dev, buf, 64);
    if (response) memcpy(response, buf, 64);
    return res;
}

void set_color(hid_device* dev, int zones, int r, int g, int b)
{
    unsigned char rgb_data[256], buf[65];
    int len = zones * 3;

    for (int i = 0; i < zones; i++) {
        rgb_data[i * 3] = (i < 20) ? g : r;
        rgb_data[i * 3 + 1] = (i < 20) ? r : b;
        rgb_data[i * 3 + 2] = (i < 20) ? b : g;
    }

    memset(buf, 0, 65);
    buf[0] = 0x00;
    buf[1] = 0x02;
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;
    memcpy(&buf[4], rgb_data, 60);
    hid_write(dev, buf, 65);

    memset(buf, 0, 65);
    buf[0] = 0x00;
    memcpy(&buf[1], &rgb_data[60], 64);
    hid_write(dev, buf, 65);

    memset(buf, 0, 65);
    buf[0] = 0x00;
    memcpy(&buf[1], &rgb_data[124], 38);
    hid_write(dev, buf, 65);

    SLEEP_MS(100);
    hid_read(dev, buf, 64);
}

void set_colors_array(hid_device* dev, int zones, unsigned char* colors)
{
    unsigned char rgb_data[256], buf[65];
    int len = zones * 3;

    for (int i = 0; i < zones; i++) {
        int r = colors[i * 3];
        int g = colors[i * 3 + 1];
        int b = colors[i * 3 + 2];

        rgb_data[i * 3] = (i < 20) ? g : r;
        rgb_data[i * 3 + 1] = (i < 20) ? r : b;
        rgb_data[i * 3 + 2] = (i < 20) ? b : g;
    }

    memset(buf, 0, 65);
    buf[0] = 0x00;
    buf[1] = 0x02;
    buf[2] = (len >> 8) & 0xFF;
    buf[3] = len & 0xFF;
    memcpy(&buf[4], rgb_data, 60);
    hid_write(dev, buf, 65);

    memset(buf, 0, 65);
    buf[0] = 0x00;
    memcpy(&buf[1], &rgb_data[60], 64);
    hid_write(dev, buf, 65);

    memset(buf, 0, 65);
    buf[0] = 0x00;
    memcpy(&buf[1], &rgb_data[124], 38);
    hid_write(dev, buf, 65);

    SLEEP_MS(100);
    hid_read(dev, buf, 64);
}

void animate_breathing(hid_device* dev, int zones, int r, int g, int b, int duration_ms)
{
    int steps = 500;
    int delay = duration_ms / steps;

    for (int frame = 0; frame < steps; frame++) {
        double brightness = (sin(frame * 2 * PI / steps) + 1) / 2;
        set_color(dev, zones, (int)(r * brightness), (int)(g * brightness), (int)(b * brightness));
        SLEEP_MS(delay);
    }
}

void animate_wave(hid_device* dev, int zones, int r, int g, int b, int duration_ms)
{
    unsigned char colors[256];
    int frames = 50;
    int delay = duration_ms / frames;

    for (int frame = 0; frame < frames; frame++) {
        for (int i = 0; i < zones; i++) {
            double offset = (i + frame) * 2 * PI / zones;
            double brightness = (sin(offset) + 1) / 2;
            colors[i * 3] = (int)(r * brightness);
            colors[i * 3 + 1] = (int)(g * brightness);
            colors[i * 3 + 2] = (int)(b * brightness);
        }
        set_colors_array(dev, zones, colors);
        SLEEP_MS(delay);
    }
}

int main(int argc, char* argv[])
{
    int r = 255, g = 255, b = 255;
    int do_breathing = 0, do_wave = 0;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--color") == 0 && i + 1 < argc) {
            if (sscanf(argv[i + 1], "%d,%d,%d", &r, &g, &b) != 3) {
                printf("Invalid color format. Use: --color r,g,b (e.g., --color 255,0,0)\n");
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "--breathing") == 0) {
            do_breathing = 1;
        } else if (strcmp(argv[i], "--wave") == 0) {
            do_wave = 1;
        }
    }

    hid_device* dev = hid_open(VID, PID, NULL);
    if (!dev) {
        printf("Failed to open device\n");
        return 1;
    }

    hid_set_nonblocking(dev, 1);

    unsigned char response[64] = { 0 }, on = 1, brightness = 255;
    send_command(dev, 0x03, NULL, 0, response);
    int zones = response[4];
    printf("Number of LED's in strip: %d\n", zones);

    send_command(dev, 0x07, &on, 1, NULL);
    send_command(dev, 0x09, &brightness, 1, NULL);

    if (do_breathing) {
        printf("Running breathing animation with color (%d,%d,%d) (Ctrl+C to stop)...\n", r, g, b);
        while (1) {
            animate_breathing(dev, zones, r, g, b, 3000);
        }
    } else if (do_wave) {
        printf("Running wave animation with color (%d,%d,%d) (Ctrl+C to stop)...\n", r, g, b);
        while (1) {
            animate_wave(dev, zones, r, g, b, 2000);
        }
    } else {
        printf("Setting solid color (%d,%d,%d)\n", r, g, b);
        set_color(dev, zones, r, g, b);
    }

    hid_close(dev);
    hid_exit();
    return 0;
}
