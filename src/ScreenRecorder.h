#ifndef SCREEN_RECORDER_SCREENRECORDER_H
#define SCREEN_RECORDER_SCREENRECORDER_H

#include <atomic>
#include <iostream>
#include <memory>
#include <functional>
#include <map>
#include <thread>
#include <mutex>
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

class ScreenRecorder {
public:
    int rec_type;
    std::atomic_bool finished, recording, pause, writing;

    ScreenRecorder();
    void open_();
    void start_();
    void pause_();
    void restart_();
    void stop_();

private:
    long vPTS, aPTS;
    int in_v_index, in_a_index;
    FormatContext v_inFmtCtx, a_inFmtCtx, outFmtCtx;
    AVAudioFifo* audioFifo;
    SwsContext* sws_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;

    std::mutex m;
    std::mutex write_lock;
    std::condition_variable cv;

    std::thread* audioThread;
    std::thread* videoThread;

    void init();
    int readFrame(AVFormatContext* fmtCtx, AVPacket* pkt);
    int writeHeader(FormatContext& fmtCtx);
    int writeTrailer(FormatContext& fmtCtx);
    void writeFrame(FormatContext& fmtCtx, Packet& pkt, AVMediaType mediaType);
    int prepareDecoder(FormatContext& fmtCtx, AVMediaType mediaType); //if ret<0 -> failed
    int prepareEncoder(FormatContext& inFmtCtx, FormatContext& outFmtCtx, AVMediaType mediaType);
    void generateOutStreams(FormatContext& outFmtCtx, const CodecContext& cCtx, const AVMediaType& mediaType);
    int sendPacket(FormatContext& inFmtCtx, FormatContext& outFmtCtx, AVPacket* pkt);
    void decode(FormatContext& inFmtCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType);
    void passFrame(Frame& frame, FormatContext& inCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType);
    void encode(FormatContext& outFmtCtx, Frame& frame, const AVMediaType& mediaType);


    void PSRMenu(); //pause-stop-restart menu
    void showPSROptions();
    static int getPSRAnswer();
    static bool validPSRAnswer(std::string &answer, int &res);
    void getFilenameOut(std::string& str);

};

#endif //SCREEN_RECORDER_SCREENRECORDER_H
