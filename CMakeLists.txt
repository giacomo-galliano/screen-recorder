cmake_minimum_required(VERSION 3.18.4)

project(screen-recorder)

set(CMAKE_CXX_STANDARD 17)

IF (WIN32)
    MESSAGE(STATUS "Platform is Windows")
    add_definitions(-DWINDOWS)
ELSEIF (APPLE)
    MESSAGE(STATUS "Platform is MacOS")
    add_definitions(-DMACOS)

    include_directories(/usr/local/include/)
    link_directories(/usr/local/lib/)
ELSEIF (UNIX)
    MESSAGE(STATUS "Platform is UNIX")
    add_definitions(-DUNIX)
ENDIF ()

add_executable(
        screen-recorder src/main.cpp
        src/ScreenRecorder.cpp src/ScreenRecorder.h)

target_link_libraries(screen-recorder
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swscale
        swresample
        )

IF(CMAKE_SYSTEM MATCHES Linux)
    target_link_libraries(${PROJECT_NAME} pthread)
ENDIF()