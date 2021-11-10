#ifndef SCREEN_RECORDER_AUDIORECORDER_H
#define SCREEN_RECORDER_AUDIORECORDER_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <inttypes.h>
#include <libavutil/imgutils.h>
}

class AudioRecorder {

    int audioIndex;

    AVStream *inAudioStream;

    AVCodecContext *aDecoderCtx, *aEncoderCtx;
    AVCodec *aDecoder, *aEncoder;

    AVPacket *aInPacket, *aOutPacket;
    AVFrame *aFrame, *aFrameConv;
    
};


#endif //SCREEN_RECORDER_AUDIORECORDER_H
