#ifndef SCREEN_RECORDER_COM_H
#define SCREEN_RECORDER_COM_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <inttypes.h>
#include <libavutil/imgutils.h>
#include <libavutil/audio_fifo.h>
}

#endif //SCREEN_RECORDER_COM_H
