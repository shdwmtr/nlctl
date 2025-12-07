#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstdint>

class ScreenCapture
{
  public:
    ScreenCapture();
    ~ScreenCapture();

    bool capture(float percent = 1.0f); // 0.0 to 1.0

    uint8_t* data() const
    {
        return img_data_;
    }
    int width() const
    {
        return capture_width_;
    }
    int height() const
    {
        return capture_height_;
    }
    int bytes_per_pixel() const
    {
        return ximg_ ? ximg_->bits_per_pixel / 8 : 4;
    }

  private:
    Display* dpy_;
    Window root_;
    int screen_width_, screen_height_;
    int capture_width_, capture_height_;
    int capture_x_, capture_y_;
    uint8_t* img_data_;
    XImage* ximg_;
    bool use_shm_;
    XShmSegmentInfo shminfo_;
    float last_percent_;

    void cleanup();
    void reallocate_shm(int width, int height);
};
