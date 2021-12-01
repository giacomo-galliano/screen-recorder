#ifndef SCREEN_RECORDER_FORMATCONTEXT_H
#define SCREEN_RECORDER_FORMATCONTEXT_H

#include <iostream>
#include <memory>
#include <map>

#include "com.h"
#include "Packet.h"
#include "CodecContext.h"

using FormatContextBase = std::unique_ptr<AVFormatContext, void(*)(AVFormatContext*)>;
class FormatContext : public FormatContextBase {
public:
    FormatContext();
    template <class T>
    FormatContext(AVFormatContext* fmtCtx, T customDeleter): FormatContextBase(fmtCtx, customDeleter){};

    Packet end(){return Packet();};
    Packet begin(){return Packet(get());};
    std::map<int, CodecContext> open_streams;
};

FormatContext openInput(const std::string& url, const std::string& ift_short_name, AVDictionary* options);
FormatContext openOutput(const std::string& filename_out);

#endif //SCREEN_RECORDER_FORMATCONTEXT_H
