#pragma once
#include <cstdint>
#include <memory>

class ScreenCaptureImpl;

class ScreenCapture
{
  public:
    ScreenCapture();
    ~ScreenCapture();

    ScreenCapture(const ScreenCapture&) = delete;
    ScreenCapture& operator=(const ScreenCapture&) = delete;

    bool capture(float percent = 1.0f);
    uint8_t* data() const;
    int width() const;
    int height() const;
    int bytes_per_pixel() const;

  private:
    std::unique_ptr<ScreenCaptureImpl> impl_;
};