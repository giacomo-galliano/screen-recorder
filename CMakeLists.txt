cmake_minimum_required(VERSION 3.18.4)
project(screen-recorder)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(
        screen-recorder src/main.cpp
        src/ScreenRecorder.cpp src/ScreenRecorder.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavcodec
        libavdevice
        libavfilter
        libavformat
        libavutil
        libswscale
        libswresample
        )

target_link_libraries(${PROJECT_NAME}
        PkgConfig::LIBAV
        )
