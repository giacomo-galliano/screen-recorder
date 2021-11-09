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
#include <iostream>

typedef struct StreamingParams{
    char videoCodec;
    char audioCodec;

}StreamingParams;

typedef  struct StreamingContext {
    AVFormatContext *fctx;
    AVCodec *videoCodec;
    AVCodec *audioCodec;
    AVStream *videoStream;
    AVStream *audioStream;
    AVCodecContext *videoCodecContex;
    int videoIndex;
    int audioIndex;
}StreamingContext;

class ScreenRecorder {

    char *outFilename;

    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVInputFormat* ift;

    AVDictionary *muxerOptions;

    int videoIndex, audioIndex;

    AVStream *inVideoStream, *inAudioStream, *outVideoStream, *outAudioStream;

    AVCodecContext *decoderCCtx, *encoderCCtx;
    AVCodec *decoderC, *encoderC;

    AVPacket *inPacket, *outPacket;
    AVFrame *frame, *frameConv;

    int fillStreamInfo();

public:
    ScreenRecorder();
    ~ScreenRecorder();
    int PrepareDecoder();
    int openInput();
};


#endif //SCREEN_RECORDER_SCREENRECORDER_H
