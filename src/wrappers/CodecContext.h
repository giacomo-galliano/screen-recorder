#ifndef SCREEN_RECORDER_CODECCONTEXT_H
#define SCREEN_RECORDER_CODECCONTEXT_H

#include "wrappers.h"

using CodecContext = std::unique_ptr<AVCodecContext, void(*)(AVCodecContext*)>;

#endif //SCREEN_RECORDER_CODECCONTEXT_H
