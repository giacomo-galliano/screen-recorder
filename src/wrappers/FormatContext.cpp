#include "FormatContext.h"

FormatContext::FormatContext():FormatContextBase(nullptr, [](AVFormatContext*){}){};

FormatContext openInput(const std::string& url, const std::string& ift_short_name, AVDictionary* options){
    AVFormatContext* inFmtCtx = nullptr;
    //inserire variabile a seconda dell'ambiente di esecuzione
    AVInputFormat* ift = av_find_input_format(ift_short_name.c_str());

    //inFmtCtx->probesize = 40000000;

    if(!ift){
        std::cout << "Unable to find input format: " << ift_short_name << std::endl;
        return FormatContext();
    }
    int err = avformat_open_input(&inFmtCtx, url.c_str(), ift, &options);
    if(err<0 || !inFmtCtx){
        return FormatContext();
    }

    err = avformat_find_stream_info(inFmtCtx, &options);
    if(err < 0){
        avformat_close_input(&inFmtCtx);
        return FormatContext();
    }


    return FormatContext(inFmtCtx, [](AVFormatContext* inCtx){
        /*
        AVFormatContext** prev_ctx = &inCtx;
        avformat_close_input(prev_ctx);
        */
        avformat_free_context(inCtx);
    });
}

FormatContext openOutput(const std::string& filename_out){
    AVFormatContext* outFmtCtx = nullptr;

    AVOutputFormat* oft = av_guess_format(nullptr, filename_out.c_str(), nullptr);
    if(!oft){
        std::cout << "Problems occurred while guessing output format." << std::endl;
        return FormatContext();
    }
    int err = avformat_alloc_output_context2(&outFmtCtx, oft, NULL, filename_out.c_str());
    if(err != 0 || !outFmtCtx){
        return FormatContext();
    }

    err = avio_open2(&outFmtCtx->pb, filename_out.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if(err < 0){
        avformat_free_context(outFmtCtx);
        return FormatContext();
    }

    return FormatContext(outFmtCtx, [](AVFormatContext* outCtx){
        avio_close(outCtx->pb);
        avformat_free_context(outCtx);
    });
};