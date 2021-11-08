#ifndef SCREEN_RECORDER_SCREENRECORDER_H
#define SCREEN_RECORDER_SCREENRECORDER_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <inttypes.h>
#include <libavutil/imgutils.h>
}


class ScreenRecorder {

    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVInputFormat* ift;

    int videoIndex, audioIndex;

    AVStream *inVideoStream, *inAudioStream, *outVideoStream, *outAudioStream;

    AVCodecContext *decoderCtx, *encoderCtx;
    AVCodec *decoder, *encoder;

    AVPacket *inPacket, *outPacket;
    AVFrame *frame, *frameConv;

public:
    ScreenRecorder();
    ~ScreenRecorder();
};

bool sr_init();
bool sr_decode();
bool sr_store();
void sr_clear();


#endif //SCREEN_RECORDER_SCREENRECORDER_H
