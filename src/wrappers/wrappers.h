#ifndef SCREEN_RECORDER_WRAPPERS_H
#define SCREEN_RECORDER_WRAPPERS_H

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
#include <memory>
#include <functional>
#include "FormatContext.h"
#include "Packet.h"
#include "CodecContext.h"
#include "Frame.h"

/*
 * the idea is to immediately attach the deleter, so that when the unique_ptr goes out of scope,
 * it will automatically call the functions needed to avoid memory leaks
 */


/*
 * std::unique_ptr has a second template parameter, its deleter that has a default type std::default_delete<T>
 * is a function object that calls delete on the object when invoked. Delete can be replaced with a custom deleter.
 * A non specialized deleter uses "delete" to deallocate memory for a single object.
 *
 * The deleter is part of the type of unique_ptr.
 * And since the functor/lambda that is stateless, its type fully encodes everything there is to know about this without any size involvement.
 * Using function pointer takes one pointer size and std::function takes even more size.
 */

SwsContext* sws_ctx = nullptr;
SwrContext* swr_ctx = nullptr;

void init();
int readFrame(AVFormatContext* fmtCtx, AVPacket* pkt);
int prepareDecoder(FormatContext* fmtCtx, AVMediaType mediaType); //if ret<0 -> failed
int prepareEncoder(FormatContext* fmtCtx, AVMediaType mediaType);
int sendPacket(FormatContext& fmtCtx, const AVPacket* pkt, std::function<void(Frame&)>passFrame);
int sendPacket(FormatContext& fmtCtx, Packet&pkt, std::function<void(Frame&)>pFrame);
void decode(FormatContext& fmtCtx);
void passFrame(std::unique_ptr<AVFrame>& frame);
#endif //SCREEN_RECORDER_WRAPPERS_H
