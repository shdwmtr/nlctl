#include "x11.hpp"
#include <algorithm>

ScreenCapture::ScreenCapture() : dpy_(nullptr), img_data_(nullptr), ximg_(nullptr), use_shm_(false), last_percent_(0.0f)
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

ScreenCapture::~ScreenCapture()
{
    cleanup();
    if (dpy_) XCloseDisplay(dpy_);
}

void ScreenCapture::cleanup()
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

void ScreenCapture::reallocate_shm(int width, int height)
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

bool ScreenCapture::capture(float percent)
{
    if (!dpy_) return false;

    percent = std::max(0.01f, std::min(1.0f, percent));

    // Calculate centered capture region
    capture_width_ = static_cast<int>(screen_width_ * percent);
    capture_height_ = static_cast<int>(screen_height_ * percent);
    capture_x_ = (screen_width_ - capture_width_) / 2;
    capture_y_ = (screen_height_ - capture_height_) / 2;

    if (use_shm_) {
        // Reallocate if size changed
        if (percent != last_percent_) {
            reallocate_shm(capture_width_, capture_height_);
            last_percent_ = percent;
        }

        if (!ximg_) return false;

        // Fast path: MIT-SHM with offset
        if (!XShmGetImage(dpy_, root_, ximg_, capture_x_, capture_y_, AllPlanes)) {
            return false;
        }
        img_data_ = reinterpret_cast<uint8_t*>(ximg_->data);
        return true;
    } else {
        // Fallback: XGetImage with region
        if (ximg_) XDestroyImage(ximg_);

        ximg_ = XGetImage(dpy_, root_, capture_x_, capture_y_, capture_width_, capture_height_, AllPlanes, ZPixmap);
        if (!ximg_) return false;

        img_data_ = reinterpret_cast<uint8_t*>(ximg_->data);
        last_percent_ = percent;
        return true;
    }
}
