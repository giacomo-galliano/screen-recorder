cmake_minimum_required(VERSION 3.18.4)
project(screen-recorder)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(
        screen-recorder src/main.cpp
        src/wrappers/com.h
        src/wrappers/FormatContext.cpp src/wrappers/FormatContext.h
        src/wrappers/Packet.cpp src/wrappers/Packet.h
        src/wrappers/wrappers.cpp src/wrappers/wrappers.h
        src/wrappers/Frame.cpp src/wrappers/Frame.h
        src/wrappers/CodecContext.cpp src/wrappers/CodecContext.h
        src/SettingsConf.cpp src/SettingsConf.h src/ScreenRecorder.cpp src/ScreenRecorder.h)

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
