#ifndef SCREEN_RECORDER_CODECCONTEXT_H
#define SCREEN_RECORDER_CODECCONTEXT_H

#include <memory>

#include "com.h"

using CodecContext = std::unique_ptr<AVCodecContext, void(*)(AVCodecContext*)>;

#endif //SCREEN_RECORDER_CODECCONTEXT_H
