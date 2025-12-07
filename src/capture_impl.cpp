#include "capture.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <vector>
#include <algorithm>

class ScreenCaptureImpl
{
  public:
    HDC hdc_screen_;
    HDC hdc_mem_;
    HBITMAP hbitmap_;
    int screen_width_, screen_height_;
    int capture_width_, capture_height_;
    std::vector<uint8_t> buffer_;
    float last_percent_;

    ScreenCaptureImpl() : hdc_screen_(nullptr), hdc_mem_(nullptr), hbitmap_(nullptr), last_percent_(0.0f)
    {
        hdc_screen_ = GetDC(nullptr);
        screen_width_ = GetDeviceCaps(hdc_screen_, HORZRES);
        screen_height_ = GetDeviceCaps(hdc_screen_, VERTRES);
        hdc_mem_ = CreateCompatibleDC(hdc_screen_);
    }

    ~ScreenCaptureImpl()
    {
        if (hbitmap_) DeleteObject(hbitmap_);
        if (hdc_mem_) DeleteDC(hdc_mem_);
        if (hdc_screen_) ReleaseDC(nullptr, hdc_screen_);
    }

    bool capture(float percent)
    {
        percent = max(0.01f, min(1.0f, percent));
        capture_width_ = static_cast<int>(screen_width_ * percent);
        capture_height_ = static_cast<int>(screen_height_ * percent);
        int capture_x = (screen_width_ - capture_width_) / 2;
        int capture_y = (screen_height_ - capture_height_) / 2;

        if (percent != last_percent_) {
            if (hbitmap_) DeleteObject(hbitmap_);
            hbitmap_ = CreateCompatibleBitmap(hdc_screen_, capture_width_, capture_height_);
            last_percent_ = percent;
        }

        SelectObject(hdc_mem_, hbitmap_);
        if (!BitBlt(hdc_mem_, 0, 0, capture_width_, capture_height_, hdc_screen_, capture_x, capture_y, SRCCOPY)) return false;

        BITMAPINFOHEADER bi = {};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = capture_width_;
        bi.biHeight = -capture_height_;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;

        buffer_.resize(capture_width_ * capture_height_ * 4);
        return GetDIBits(hdc_mem_, hbitmap_, 0, capture_height_, buffer_.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS) != 0;
    }

    int bytes_per_pixel() const
    {
        return 4;
    }
};

#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <algorithm>

class ScreenCaptureImpl
{
  public:
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

    ScreenCaptureImpl() : dpy_(nullptr), img_data_(nullptr), ximg_(nullptr), use_shm_(false), last_percent_(0.0f)
    {
        dpy_ = XOpenDisplay(nullptr);
        if (!dpy_) return;
        root_ = DefaultRootWindow(dpy_);
        XWindowAttributes attr;
        XGetWindowAttributes(dpy_, root_, &attr);
        screen_width_ = attr.width;
        screen_height_ = attr.height;
        use_shm_ = XShmQueryExtension(dpy_);
    }

    ~ScreenCaptureImpl()
    {
        cleanup();
        if (dpy_) XCloseDisplay(dpy_);
    }

    void cleanup()
    {
        if (use_shm_) {
            if (ximg_) {
                XShmDetach(dpy_, &shminfo_);
                XDestroyImage(ximg_);
                shmdt(shminfo_.shmaddr);
            }
        } else {
            if (ximg_) XDestroyImage(ximg_);
        }
        ximg_ = nullptr;
        img_data_ = nullptr;
    }

    void reallocate_shm(int width, int height)
    {
        cleanup();
        ximg_ = XShmCreateImage(dpy_, DefaultVisual(dpy_, DefaultScreen(dpy_)), DefaultDepth(dpy_, DefaultScreen(dpy_)), ZPixmap, nullptr, &shminfo_, width, height);
        if (ximg_) {
            shminfo_.shmid = shmget(IPC_PRIVATE, ximg_->bytes_per_line * ximg_->height, IPC_CREAT | 0777);
            if (shminfo_.shmid != -1) {
                shminfo_.shmaddr = ximg_->data = static_cast<char*>(shmat(shminfo_.shmid, 0, 0));
                if (shminfo_.shmaddr != (char*)-1) {
                    shminfo_.readOnly = False;
                    if (XShmAttach(dpy_, &shminfo_)) {
                        XSync(dpy_, False);
                        shmctl(shminfo_.shmid, IPC_RMID, 0);
                        img_data_ = reinterpret_cast<uint8_t*>(ximg_->data);
                    } else {
                        use_shm_ = false;
                        cleanup();
                    }
                } else {
                    shmctl(shminfo_.shmid, IPC_RMID, 0);
                }
            }
        }
    }

    bool capture(float percent)
    {
        if (!dpy_) return false;
        percent = std::max(0.01f, std::min(1.0f, percent));
        capture_width_ = static_cast<int>(screen_width_ * percent);
        capture_height_ = static_cast<int>(screen_height_ * percent);
        capture_x_ = (screen_width_ - capture_width_) / 2;
        capture_y_ = (screen_height_ - capture_height_) / 2;

        if (use_shm_) {
            if (percent != last_percent_) {
                reallocate_shm(capture_width_, capture_height_);
                last_percent_ = percent;
            }
            if (!ximg_) return false;
            if (!XShmGetImage(dpy_, root_, ximg_, capture_x_, capture_y_, AllPlanes)) return false;
            img_data_ = reinterpret_cast<uint8_t*>(ximg_->data);
        } else {
            if (ximg_) XDestroyImage(ximg_);
            ximg_ = XGetImage(dpy_, root_, capture_x_, capture_y_, capture_width_, capture_height_, AllPlanes, ZPixmap);
            if (!ximg_) return false;
            img_data_ = reinterpret_cast<uint8_t*>(ximg_->data);
            last_percent_ = percent;
        }
        return true;
    }

    int bytes_per_pixel() const
    {
        return ximg_ ? ximg_->bits_per_pixel / 8 : 4;
    }
};
#endif

ScreenCapture::ScreenCapture() : impl_(std::make_unique<ScreenCaptureImpl>())
{
}
ScreenCapture::~ScreenCapture() = default;

bool ScreenCapture::capture(float percent)
{
    return impl_->capture(percent);
}
int ScreenCapture::width() const
{
    return impl_->capture_width_;
}
int ScreenCapture::height() const
{
    return impl_->capture_height_;
}
int ScreenCapture::bytes_per_pixel() const
{
    return impl_->bytes_per_pixel();
}

uint8_t* ScreenCapture::data() const
{
#if defined(_WIN32) || defined(_WIN64)
    return impl_->buffer_.data();
#else
    return impl_->img_data_;
#endif
}