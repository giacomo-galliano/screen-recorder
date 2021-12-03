#ifndef SCREEN_RECORDER_SCREENRECORDER_H
#define SCREEN_RECORDER_SCREENRECORDER_H

extern "C"
{
#include "../include/libavcodec/avcodec.h"
#include "../include/libavformat/avformat.h"
#include "../include/libswscale/swscale.h"
#include "../include/libswresample/swresample.h"
#include "../include/libavdevice/avdevice.h"
#include <inttypes.h>
#include "../include/libavutil/imgutils.h"
#include "../include/libavutil/audio_fifo.h"
}
#include <iostream>

static const int64_t FLICKS_TIMESCALE = 705600000;
static const ::AVRational FLICKS_TIMESCALE_Q = { 1, FLICKS_TIMESCALE };

class ScreenRecorder {


    const AVSampleFormat requireAudioFmt = AV_SAMPLE_FMT_FLTP;

    char *outFilename;

    AVFormatContext *videoInFormatCtx, *audioInFormatCtx;
    AVFormatContext *outFormatCtx;
    AVInputFormat *vIft, *aIft;
    AVOutputFormat *oft;

    AVDictionary *muxerOptions;

    int videoIndex, audioIndex;
    int64_t vpts, apts, vdts, last_pts, last_apts;

    AVStream *outVideoStream, *outAudioStream;

    AVCodecContext *vDecoderCCtx, *vEncoderCCtx, *aDecoderCCtx, *aEncoderCCtx;
    AVCodec *vDecoderC, *vEncoderC, *aDecoderC, *aEncoderC;

    AVAudioFifo *audioFifo;

    AVPacket *inPacket, *outPacket;
    AVFrame *inFrame, *convFrame;

    int fillStreamInfo();
    int transcodeVideo(int* index, SwsContext *pContext);
    int transcodeAudio(int* indexFrame, SwrContext *pContext);
    int encode(int* i, int streamIndex, AVCodecContext* cctx, AVStream* outStream);
    void flushAll();

public:
    ScreenRecorder();
    ~ScreenRecorder();
    int PrepareDecoder();
    int openInput();
    int prepareEncoder();
    int openOutput();
    int writeHeader();
    int decoding();
    int writeTrailer();
};


#endif //SCREEN_RECORDER_SCREENRECORDER_H
