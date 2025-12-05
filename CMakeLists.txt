cmake_minimum_required(VERSION 3.14)
project(nlctl)

set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS OFF)

include(FetchContent)

FetchContent_Declare(
    hidapi
    GIT_REPOSITORY https://github.com/libusb/hidapi
    GIT_TAG master
)

FetchContent_MakeAvailable(hidapi)
add_executable(nlctl main.cc)
target_link_libraries(nlctl PRIVATE hidapi)