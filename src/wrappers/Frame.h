#ifndef SCREEN_RECORDER_FRAME_H
#define SCREEN_RECORDER_FRAME_H

#include <memory>

#include "com.h"

/*
using FrameBase = std::unique_ptr<AVFrame, void(*)(AVFrame *)>;
class Frame : FrameBase{
public:
    Frame() : FrameBase(av_frame_alloc(), [](AVFrame* frame){
        av_frame_free(&frame);
    }){};

};
 */
using Frame = std::unique_ptr<AVFrame, void(*)(AVFrame *)>;

Frame frameAlloc();

#endif //SCREEN_RECORDER_FRAME_H