#ifndef SCREEN_RECORDER_SCREENRECORDER_H
#define SCREEN_RECORDER_SCREENRECORDER_H

#include <atomic>
#include <iostream>
#include <memory>
#include <functional>
#include <map>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "./wrappers/com.h"
#include "./wrappers/FormatContext.h"
#include "./wrappers/Packet.h"
#include "./wrappers/CodecContext.h"
#include "./wrappers/Frame.h"
#include "SettingsConf.h"

#define OUT_VIDEO_INDEX 0
#define OUT_AUDIO_INDEX 1

inline const AVSampleFormat requireAudioFmt = AV_SAMPLE_FMT_FLTP;

enum RecStatus{RECORDING, RESTARTING,  PAUSE, STOP};

using namespace  std;

class ScreenRecorder {
public:


    int rec_type;

    ScreenRecorder();
    ~ScreenRecorder();
    void open_();
    void start_();
    void pause_();
    void restart_();
    void stop_();

private:

    //VIDEO VARIABLES
    long vPTS;
    int inVIndex;
    AVDictionary * sourceOptions;
    AVInputFormat *inVFmt;
    AVFormatContext* inVFmtCtx;
    AVCodec* inVC;
    AVCodecContext* inVCCtx;
    AVCodec* outVC;
    AVCodecContext* outVCCtx;
    AVStream* outVStream;
    SwsContext* swsCtx;
    AVFrame* convFrame;





    long aPTS;
    int inAIndex;
    AVFormatContext *outFmtCtx;
    AVOutputFormat* outFmt;
    AVAudioFifo* audioFifo;
    string outFileName;
    SwrContext* swr_ctx = nullptr;

    mutex video_queue_mutex;
    queue<AVPacket *> video_queue;

    mutex status_lock;
    condition_variable cv;

    mutex write_lock;


    thread* audioThread;
    thread* videoThread;
    thread* readVideoThread;

    RecStatus status;


    void writeHeader();
    void writeTrailer();
    void openVideoInput();
    void initVideoEncoder();
    void readFrame();
    void processVideo();
    void processAudio();



    void PSRMenu(); //pause-stop-restart menu
    void showPSROptions();
    static int getPSRAnswer();
    static bool validPSRAnswer(std::string &answer, int &res);
    void getFilenameOut(std::string& str);

};

#endif //SCREEN_RECORDER_SCREENRECORDER_H
