cmake_minimum_required(VERSION 3.14)
project(nlctl)

set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

include(FetchContent)
FetchContent_Declare(
    hidapi
    GIT_REPOSITORY https://github.com/libusb/hidapi
    GIT_TAG master
)
FetchContent_MakeAvailable(hidapi)

include_directories(include)
add_executable(nlctl
    src/main.cpp
    src/animation_base.cpp
    src/color.cpp
    src/hid_device.cpp
    src/x11.cpp
    src/zone.cpp
)

if (UNIX)
    target_link_libraries(nlctl PRIVATE hidapi::hidraw X11 Xext GL)
else()
    target_link_libraries(nlctl PRIVATE hidapi)
endif()
