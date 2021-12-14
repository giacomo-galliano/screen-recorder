cmake_minimum_required(VERSION 3.18.4)

project(screen-recorder)

set(CMAKE_CXX_STANDARD 17)

#file(GLOB_RECURSE libav CONFIGURE_DEPENDS
#        ${PROJECT_SOURCE_DIR}/include/*.h
#        ${PROJECT_SOURCE_DIR}/include/*.cpp
#        )

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

    include_directories(${PROJECT_SOURCE_DIR}/include)
    link_directories(${PROJECT_SOURCE_DIR}/lib)
ENDIF ()

add_executable(
        screen-recorder src/main.cpp
        src/ScreenRecorder.cpp src/ScreenRecorder.h)

#find_package(PkgConfig REQUIRED)
#pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
#        libavcodec
#        libavdevice
#        libavfilter
#        libavformat
#        libavutil
#        libswscale
#        libswresample
#        )
#
target_link_libraries(screen-recorder
        ${PROJECT_SOURCE_DIR}/lib/libavcodec.a
        ${PROJECT_SOURCE_DIR}/lib/libavdevice.a
        ${PROJECT_SOURCE_DIR}/lib/libavfilter.a
        ${PROJECT_SOURCE_DIR}/lib/libavformat.a
        ${PROJECT_SOURCE_DIR}/lib/libavutil.a
        ${PROJECT_SOURCE_DIR}/lib/libswresample.a
        ${PROJECT_SOURCE_DIR}/lib/libswscale.a)
#        ${PROJECT_SOURCE_DIR}/lib/libx265.a
#        ${PROJECT_SOURCE_DIR}/lib/libx264.a
#        ${PROJECT_SOURCE_DIR}/lib/libvpx.a
#        ${PROJECT_SOURCE_DIR}/lib/libvmaf.a
#        ${PROJECT_SOURCE_DIR}/lib/libSvtAv1Enc.a
#        ${PROJECT_SOURCE_DIR}/lib/libpostproc.a
#        ${PROJECT_SOURCE_DIR}/lib/libopus.la
#        ${PROJECT_SOURCE_DIR}/lib/libopus.a
#        ${PROJECT_SOURCE_DIR}/lib/libfdk-aac.la
#        ${PROJECT_SOURCE_DIR}/lib/libfdk-aac.a
#        ${PROJECT_SOURCE_DIR}/lib/libdav1d.a)

#target_link_libraries(screen-recorder
#    avcodec
#    avdevice
#    avfilter
#    avformat
#    avutil
#    swscale
#    swresample
#)


#target_link_libraries(${PROJECT_NAME}
#        PkgConfig::LIBAV
#        )
