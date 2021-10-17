cmake_minimum_required(VERSION 3.20)
project(screen-recorder)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(
        src/main.cpp
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavcodec
        libavdevice
        libavfilter
        libavformat
        libavutil
        libswscale
        )

target_link_libraries(${PROJECT_NAME}
        PkgConfig::LIBAV
        )
