#ifndef SCREEN_RECORDER_ORCHESTRATOR_H
#define SCREEN_RECORDER_ORCHESTRATOR_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <inttypes.h>
#include <libavutil/imgutils.h>
}

#include "AudioRecorder.h";
#include "VideoRecorder.h";

class Orchestrator {
    AVFormatContext *inFormatCtx, *outFormatCtx;
    AVInputFormat* ift;
    AVOutputFormat* oft;

public:
    Orchestrator();
    ~Orchestrator();


};


#endif //SCREEN_RECORDER_ORCHESTRATOR_H
