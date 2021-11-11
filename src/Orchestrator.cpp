//
// Created by giacomo on 10/11/21.
//

#include "Orchestrator.h"

Orchestrator::Orchestrator(){

}

void Orchestrator::openInput(const std::string& url){
    AVFormatContext* fmtCtx = nullptr;
    AVInputFormat* ift = av_find_input_format("x11grab");

    int err = avformat_open_input(&fmtCtx, url.c_str(), ift, nullptr);
    if(err<0 || !fmtCtx){
        std::cout << "Cannot open input fomat." << std::endl;
        exit(1);
    }

    err = avformat_find_stream_info(fmtCtx, nullptr);
    if(err<0){
        avformat_close_input(&fmtCtx);
        std::cout << "Cannot find stream info." << std::endl;
        exit(1);
    }

    auto deleter = [](AVFormatContext* ctx){
        auto p_ctx = &ctx;
        avformat_close_input(p_ctx);
    };
    return std::unique_ptr<AVFormatContext, decltype(deleter)> up (fmtCtx, deleter );

}