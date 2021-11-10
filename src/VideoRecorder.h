#ifndef SCREEN_RECORDER_VIDEORECORDER_H
#define SCREEN_RECORDER_VIDEORECORDER_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <inttypes.h>
#include <libavutil/imgutils.h>
}


class VideoRecorder {

    int videoIndex;

    AVStream *inVideoStream;

    AVCodecContext *vDecoderCtx, *vEncoderCtx;
    AVCodec *vDecoder, *vEncoder;

    AVPacket *vInPacket, *vOutPacket;
    AVFrame *vFrame, *vFrameConv;

public:
    VideoRecorder();
    ~VideoRecorder();
};

#endif //SCREEN_RECORDER_VIDEORECORDER_H
