#ifndef SCREEN_RECORDER_WRAPPERS_H
#define SCREEN_RECORDER_WRAPPERS_H

#include <iostream>
#include <memory>
#include <functional>
#include <map>

#include "com.h"
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

//////////////////////////////////////////////////////// IN common.h
const AVSampleFormat requireAudioFmt = AV_SAMPLE_FMT_FLTP;
inline AVAudioFifo* audioFifo;
////////////////////////////////////////////////////////
inline SwsContext* sws_ctx = nullptr;
inline SwrContext* swr_ctx = nullptr;

void init();
int readFrame(AVFormatContext* fmtCtx, AVPacket* pkt);
int writeHeader(FormatContext& fmtCtx);
int writeTrailer(FormatContext& fmtCtx);
int writeFrame(FormatContext& fmtCtx, const Packet& pkt, AVMediaType mediaType);
int prepareDecoder(FormatContext& fmtCtx, AVMediaType mediaType); //if ret<0 -> failed
int prepareEncoder(FormatContext& inFmtCtx, FormatContext& outFmtCtx, AVMediaType mediaType);
int sendPacket(FormatContext& inFmtCtx, FormatContext& outFmtCtx, const AVPacket* pkt);
int sendPacket(FormatContext& inFmtCtx, FormatContext& outFmtCtx, Packet&pkt);
void decode(FormatContext& inFmtCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType);
void passFrame(Frame& frame, FormatContext& inCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType);
void encode(FormatContext& outFmtCtx, Frame& frame, const AVMediaType& mediaType);
void generateOutStreams(FormatContext& outFmtCtx, const CodecContext& cCtx, const AVMediaType& mediaType);

#endif //SCREEN_RECORDER_WRAPPERS_H
