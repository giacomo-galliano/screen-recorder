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

int ScreenRecorder::prepareVideoEncoder() {
    av_guess_format(nullptr, outFilename, nullptr);
    if(!oft){
        std::cout << "Can't create output format" << std::endl;
        return -1;
    }
    avformat_alloc_output_context2(&outFormatCtx, oft, NULL, outFilename);
    outVideoStream = avformat_new_stream(outFormatCtx, NULL);
    if (!outFormatCtx) {
        std::cout << "Couldn't create output AVFormatContext" << std::endl;
        return -2;
    }
    encoderC = avcodec_find_encoder((AV_CODEC_ID_MPEG4));
    if(!encoderC){
        std::cout << "An error occurred trying to find the encoder codec" << std::endl;
        return -3;
    }

    encoderCCtx = avcodec_alloc_context3(encoderC);
    if (!encoderCCtx) {
        std::cout << "Error allocating encoder context" << std::endl;
        return -4;
    }

    encoderCCtx = outVideoStream->codec;
    encoderCCtx->codec_id = AV_CODEC_ID_MPEG4;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
    encoderCCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    encoderCCtx->pix_fmt  = AV_PIX_FMT_YUV420P;
    encoderCCtx->bit_rate = 2500000;
    encoderCCtx->width = 1920;
    encoderCCtx->height = 1080;
    encoderCCtx->gop_size = 3;
    encoderCCtx->max_b_frames = 2;
    encoderCCtx->time_base.num = 1;
    encoderCCtx->time_base.den = 150; // 30->15fps

    /* Some container formats (like MP4) require global headers to be present
    Mark the encoder so that it behaves accordingly. */
    if ( outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        outFormatCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(encoderCCtx, encoderC, nullptr) < 0) {
        std::cout << "Could not open the encoder." << std::endl;
        decoderCCtx = nullptr;
        return -5;
    }
    avcodec_parameters_from_context(outVideoStream->codecpar, encoderCCtx);



    return 0;
}

int ScreenRecorder::openOutput() {
    if (!(oft->flags & AVFMT_NOFILE)) {
        //avio_open2(&outFormatCtx->pb, filename_out, AVIO_FLAG_WRITE, NULL, NULL);
        if ( avio_open(&outFormatCtx->pb, outFilename, AVIO_FLAG_WRITE) < 0) {
            std::cout << "Could not open output file " << outFilename<< std::endl;
            return -1;
        }
    }
    return 0;
}

int ScreenRecorder::writeHeader() {

    if(av_dict_set(&muxerOptions, "framerate", "48", 0) != 0){
        std::cout << "Error setting dictionary value" << std::endl;
        return -1;
    }

    if(av_dict_set(&muxerOptions, "preset", "medium", 0) != 0){
        std::cout << "Error setting dictionary value" << std::endl;
    }


    if (avformat_write_header(outFormatCtx, &muxerOptions) < 0) {
        std::cout << "Failed to write header" << std::endl;
        return -2;
    }

    av_dump_format(outFormatCtx, 0, outFilename, 1);

    return 0;
}




