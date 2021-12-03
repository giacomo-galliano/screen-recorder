#ifndef SCREEN_RECORDER_SCREENRECORDER_H
#define SCREEN_RECORDER_SCREENRECORDER_H

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
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

static const int64_t FLICKS_TIMESCALE = 705600000;
static const ::AVRational FLICKS_TIMESCALE_Q = { 1, FLICKS_TIMESCALE };

class ScreenRecorder {

    std::mutex m;
    std::condition_variable cv;


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

    AVPacket *inVideoPacket, *inAudioPacket, *outVideoPacket, *outAudioPacket;
    AVFrame *inVideoFrame, *inAudioFrame, *convAudioFrame, *convVideoFrame;

    int fillStreamInfo();
    int transcodeVideo(SwsContext *pContext);
    int transcodeAudio(SwrContext *pContext);
    int encode(int streamIndex, AVCodecContext* cctx, AVStream* outStream, AVFrame * convframe);
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
