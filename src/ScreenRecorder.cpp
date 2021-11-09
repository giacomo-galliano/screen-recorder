#include "ScreenRecorder.h"

ScreenRecorder::ScreenRecorder() {
    avformat_network_init();
    avdevice_register_all();

    outFilename = "../media/output.mp4";
}

ScreenRecorder::~ScreenRecorder() {

}

int ScreenRecorder::fillStreamInfo() {
    decoderC = avcodec_find_decoder(inVideoStream->codecpar->codec_id);
    if (!decoderC) {
        std::cout << "failed to find the codec." << std::endl;
        return -1;
    }
    decoderCCtx = avcodec_alloc_context3(decoderC);
    if (!decoderCCtx) {
        std::cout << "Error allocating decoder context" << std::endl;
        avformat_close_input(&inFormatCtx);
        return -1;
    }
    if(avcodec_parameters_to_context(decoderCCtx, inVideoStream->codecpar)){
        avformat_close_input(&inFormatCtx);
        avcodec_free_context(&decoderCCtx);
        return -2;
    }

    if (avcodec_open2(decoderCCtx, decoderC, nullptr) < 0) {
        std::cout << "failed to open decoder." << std::endl;
        decoderCCtx = nullptr;
        return -3;
    }
    return 0;
}

int ScreenRecorder::PrepareDecoder() {
    for (int i = 0; i < inFormatCtx->nb_streams; i++) {
        if (inFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            inVideoStream = inFormatCtx->streams[i];;
            videoIndex = i;

            if (fillStreamInfo()) {return -1;}
        } /*else if (sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            sc->audio_avs = sc->avfc->streams[i];
            sc->audio_index = i;

            if (fill_stream_info(sc->audio_avs, &sc->audio_avc, &sc->audio_avcc)) {return -1;}
        } */else {
            std::cout << "skipping streams other than audio and video." << std::endl;
        }
    }
    return 0;
}

int ScreenRecorder::openInput() {

    inFormatCtx = avformat_alloc_context();
    if (!inFormatCtx) {
        std::cout << "Couldn't create input AVFormatContext" << std::endl;
        return(-1);
    }
    ift = av_find_input_format("x11grab");
    if (avformat_open_input(&inFormatCtx, ":0.0", ift, nullptr) != 0) {
        std::cout << "Couldn't open the video file" << std::endl;
        exit(-2);
    };

    // Now we retrieve the stream informations. It populates inFormatCtx->streams with the proper infos
    if (avformat_find_stream_info(inFormatCtx, nullptr) < 0) {
        std::cout << "Couldn't find stream informations" << std::endl;
        return(-3);
    }

    return 0;
}


