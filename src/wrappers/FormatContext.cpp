#include "FormatContext.h"

FormatContext::FormatContext():FormatContextBase(nullptr, [](AVFormatContext*){}){};

FormatContext openInput(AVMediaType mediaType){
    AVFormatContext* inFmtCtx = avformat_alloc_context();
    //inserire variabile a seconda dell'ambiente di esecuzione
    AVInputFormat* ift;

    //inFmtCtx->probesize = 40000000;

#ifdef _WIN32
    ift = av_find_input_format("gdigrab");
    if (avformat_open_input(&inFmtCtx, "desktop", ift, &options) != 0) {
        cerr << "Couldn't open input stream" << endl;
        exit(-1);
    }

#elif defined linux
    int res = -1;
    AVDictionary * options = NULL;
    av_dict_set (& options, "framerate", "60", 0);
    //av_dict_set (& options, "follow_mouse", "centered", 0);
    av_dict_set (& options, "video_size", "1366x768", 0);
    if(mediaType == AVMEDIA_TYPE_VIDEO) {
        int offset_x = 0, offset_y = 0;
        std::string url = ":0.0+" + std::to_string(offset_x) + "," + std::to_string(offset_y);  //custom string to set the start point of the screen section
        ift = av_find_input_format("x11grab");
        res = avformat_open_input(&inFmtCtx, url.c_str(), ift, &options);
    }else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        ift = av_find_input_format("pulse");
        res = avformat_open_input(&inFmtCtx, "default", ift, nullptr);
    }

    if (res != 0) {
        std::cerr << "Error in opening input device (video)" << std::endl;
        exit(-1);
    }
#else

    res = av_dict_set(&options, "pixel_format", "0rgb", 0);
    if (res < 0) {
        cerr << "Error in setting pixel format" << endl;
        exit(-1);
    }

    res = av_dict_set(&options, "video_device_index", "1", 0);

    if (res < 0) {
        cerr << "Error in setting video device index" << endl;
        exit(-1);
    }

    ift = av_find_input_format("avfoundation");

    if (avformat_open_input(&inFmtCtx, "Capture screen 0:none", ift, &options) != 0) {  //TODO trovare un modo per selezionare sempre lo schermo (forse "Capture screen 0")
        cerr << "Error in opening input device" << endl;
        exit(-1);
    }

#endif
        //TODO: capire a che valore settare -> [4 * width * height * 2 + 1] (e se effetivamente serve)
    inFmtCtx->probesize = 40000000;

    res = avformat_find_stream_info(inFmtCtx, nullptr);
    if(res < 0){
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
};

FormatContext openInput(const std::string& url, const std::string& ift_short_name, AVDictionary* options){
    AVFormatContext* inFmtCtx = avformat_alloc_context();
    //inserire variabile a seconda dell'ambiente di esecuzione
    AVInputFormat* ift = av_find_input_format(ift_short_name.c_str());

    //inFmtCtx->probesize = 40000000;

    if(!ift){
        std::cout << "Unable to find input format: " << ift_short_name << std::endl;
        return FormatContext();
    }
    int res = avformat_open_input(&inFmtCtx, url.c_str(), ift, &options);
    if(res<0 || !inFmtCtx){
        return FormatContext();
    }

    res = avformat_find_stream_info(inFmtCtx, &options);
    if(res < 0){
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
    //AVFormatContext* outFmtCtx = nullptr;
    AVFormatContext* outFmtCtx = avformat_alloc_context();

    AVOutputFormat* oft = av_guess_format(nullptr, filename_out.c_str(), nullptr);
    if(!oft){
        std::cout << "Problems occurred while guessing output format." << std::endl;
        return FormatContext();
    }
    int res = avformat_alloc_output_context2(&outFmtCtx, oft, NULL, filename_out.c_str());
    if(res != 0 || !outFmtCtx){
        return FormatContext();
    }

    res = avio_open2(&outFmtCtx->pb, filename_out.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if(res < 0){
        avformat_free_context(outFmtCtx);
        return FormatContext();
    }

    return FormatContext(outFmtCtx, [](AVFormatContext* outCtx){
        avio_close(outCtx->pb);
        avformat_free_context(outCtx);
    });
};