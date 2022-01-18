#include "FormatContext.h"
using namespace std;
FormatContext::FormatContext():FormatContextBase(nullptr, [](AVFormatContext*){}){};

FormatContext openInput(AVMediaType mediaType){
    AVFormatContext* inFmtCtx = avformat_alloc_context();
    //inserire variabile a seconda dell'ambiente di esecuzione
    AVInputFormat* ift;
    AVDictionary * sourceOptions = nullptr;

    if(mediaType == AVMEDIA_TYPE_VIDEO) {

#ifdef _WIN32
    ift = av_find_input_format("gdigrab");
    if (avformat_open_input(&inFmtCtx, "desktop", ift, &options) != 0) {
        cerr << "Couldn't open input stream" << endl;
        exit(-1);
    }

#elif defined linux

//        char *displayName = getenv("DISPLAY");
//    av_dict_set (&sourceOptions, "follow_mouse", "centered", 0);
        av_dict_set (&sourceOptions, "framerate", "60", 0);
        av_dict_set (&sourceOptions, "video_size", "wxga", 0);//wxga==1366x768
        av_dict_set (&sourceOptions, "probesize", "40M", 0);
        //TODO: capire a che valore settare -> [4 * width * height * 2 + 1] (e se effetivamente serve)
//        inFmtCtx->probesize = 40000000;
        int offset_x = 0, offset_y = 0;
        std::string url = ":0.0+" + std::to_string(offset_x) + "," + std::to_string(offset_y);  //custom string to set the start point of the screen section
        ift = av_find_input_format("x11grab");
        if (ift == nullptr) {
            throw logic_error{"av_find_input_format not found..."};
        }
       if( avformat_open_input(&inFmtCtx, url.c_str(), ift, &sourceOptions) != 0) {
           throw runtime_error{"cannot open video device"};
       }
    }else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        ift = av_find_input_format("pulse");
        avformat_open_input(&inFmtCtx, "default", ift, nullptr);
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


    if(avformat_find_stream_info(inFmtCtx, &sourceOptions) < 0 ){
        throw logic_error{"cannot find correct stream info..."};
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