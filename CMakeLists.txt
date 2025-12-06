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

include_directories()

FetchContent_MakeAvailable(hidapi)
add_executable(nlctl main.cc)
if (UNIX)
    target_link_libraries(nlctl PRIVATE hidapi::hidraw)
else()
    target_link_libraries(nlctl PRIVATE hidapi)
endif()
