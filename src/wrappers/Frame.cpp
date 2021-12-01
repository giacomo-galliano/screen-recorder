#include "Frame.h"

Frame frameAlloc(){
    return Frame(av_frame_alloc(), [](AVFrame* frame) {
        av_frame_free(&frame);
    });
}
