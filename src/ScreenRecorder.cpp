#include "ScreenRecorder.h"

ScreenRecorder::ScreenRecorder() {
    avformat_network_init();
    avdevice_register_all();


    outFilename = "../media/output.mp4";
    muxerOptions = nullptr;
}

ScreenRecorder::~ScreenRecorder() {

}

int ScreenRecorder::fillStreamInfo() {

    vDecoderC = avcodec_find_decoder(inVideoStream->codecpar->codec_id);
    if (!vDecoderC) {
        std::cout << "failed to find the video codec." << std::endl;
        return -1;
    }
    vDecoderCCtx = avcodec_alloc_context3(vDecoderC);
    if (!vDecoderCCtx) {
        std::cout << "Error allocating video decoder context" << std::endl;
        avformat_close_input(&inFormatCtx);
        return -1;
    }
    if(avcodec_parameters_to_context(vDecoderCCtx, inVideoStream->codecpar)){
        avformat_close_input(&inFormatCtx);
        avcodec_free_context(&vDecoderCCtx);
        return -2;
    }

    vDecoderCCtx->time_base = (AVRational){1, 10000};// inVideoStream->time_base;
    vDecoderCCtx->framerate = av_inv_q(vDecoderCCtx->time_base);

    if (avcodec_open2(vDecoderCCtx, vDecoderC, nullptr) < 0) {
        std::cout << "failed to open video decoder." << std::endl;
        vDecoderCCtx = nullptr;
        return -3;
    }

    aDecoderC = avcodec_find_decoder(inAudioStream->codecpar->codec_id);
    if (!aDecoderC) {
        std::cout << "failed to find the audio codec." << std::endl;
        return -1;
    }
    aDecoderCCtx = avcodec_alloc_context3(aDecoderC);
    if (!aDecoderCCtx) {
        std::cout << "Error allocating audio decoder context" << std::endl;
        avformat_close_input(&audioInFormatCtx);
        return -1;
    }
    if(avcodec_parameters_to_context(aDecoderCCtx, inAudioStream->codecpar)){
        avformat_close_input(&audioInFormatCtx);
        avcodec_free_context(&aDecoderCCtx);
        return -2;
    }


    if (avcodec_open2(aDecoderCCtx, aDecoderC, nullptr) < 0) {
        std::cout << "failed to open audio decoder." << std::endl;
        aDecoderCCtx = nullptr;
        return -3;
    }

    return 0;
}


int ScreenRecorder::PrepareDecoder() {
    for (int i = 0; i < videoInFormatCtx->nb_streams; i++) {
        if (inFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            inVideoStream = inFormatCtx->streams[i];
            videoIndex = i;
        }
    }
    if(true) { //da controllare lo status
        for (int i = 0; i < audioInFormatCtx->nb_streams; i++) {
            if (audioInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                inAudioStream = audioInFormatCtx->streams[i];
                audioIndex = i;
            }
        }
        if(audioIndex==-1){
            std::cout << "Didn't find a audio stream " << std::endl;
        }
    }

    if (videoIndex == -1){
        std::cout << "Didn't find a video stream " << std::endl;
    }

    if(fillStreamInfo()){
        return -1;
    }
    return 0;
}

int ScreenRecorder::openInput() {

    inFormatCtx = avformat_alloc_context();
    if (!inFormatCtx) {
        std::cout << "Couldn't create input AVFormatContext" << std::endl;
        return(-1);
    }
    vIft = av_find_input_format("x11grab");
    aIft = av_find_input_format("pulse");
    AVDictionary* options = NULL;
//    av_dict_set(&options,"framerate","10000",0);
//    av_dict_set(&options,"video_size","wxga",0);
//    av_dict_set(&options, "crf", "12", 0);
//    inFormatCtx->probesize = 40000000;

    if (avformat_open_input(&videoInFormatCtx, ":0.0", vIft, &options) != 0) {
        std::cout << "Couldn't open the video file" << std::endl;
        exit(-2);
    };
    if (avformat_open_input(&audioInFormatCtx, "default", aIft, nullptr) != 0){
        std::cout << "Couldn't open the audio file" << std::endl;

    }

    // Now we retrieve the stream informations. It populates inFormatCtx->streams with the proper infos
    if (avformat_find_stream_info(videoInFormatCtx, nullptr) < 0) {
        std::cout << "Couldn't find stream informations" << std::endl;
        return(-3);
    }
    if (avformat_find_stream_info(audioInFormatCtx, nullptr) < 0) {
        std::cout << "Couldn't find stream informations" << std::endl;
        return(-3);
    }


    return 0;
}

int ScreenRecorder::prepareEncoder() {
    oft = av_guess_format(nullptr, outFilename, nullptr);
    if(!oft){
        std::cout << "Can't create output format" << std::endl;
        return -1;
    }
    avformat_alloc_output_context2(&outFormatCtx, oft, NULL, outFilename);

    vEncoderC = avcodec_find_encoder((AV_CODEC_ID_H264));
    if(!vEncoderC){
        std::cout << "An error occurred trying to find the video encoder codec" << std::endl;
        return -1;
    }

    vEncoderCCtx = avcodec_alloc_context3(vEncoderC);
    if (!vEncoderCCtx) {
        std::cout << "Error allocating video encoder context" << std::endl;
        return -4;
    }

    outVideoStream = avformat_new_stream(outFormatCtx, vEncoderC);
    if (!outVideoStream) {
        std::cout << "Couldn't create output video AVStream" << std::endl;
        return -1;
    }

    vEncoderCCtx->time_base = (AVRational){1, 60};
    vEncoderCCtx->framerate = (AVRational){60, 1};
    vEncoderCCtx->width = vDecoderCCtx->width;
    vEncoderCCtx->height = vDecoderCCtx->height;
    vEncoderCCtx->pix_fmt  = AV_PIX_FMT_YUV420P;
    vEncoderCCtx->codec_id = AV_CODEC_ID_H264;
    vEncoderCCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    vEncoderCCtx->gop_size = 12;
    vEncoderCCtx->bit_rate = 4000000;
//    vEncoderCCtx->level = 31;

    if (avcodec_open2(vEncoderCCtx, vEncoderC, nullptr) < 0) {
        std::cout << "Could not open the video encoder." << std::endl;
        vDecoderCCtx = nullptr;
        return -5;
    }

    if(true) {//controllo su status audio

        aEncoderC = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (!aEncoderC) {
            std::cout << "An error occurred trying to find the audio encoder codec" << std::endl;
            return -1;
        }

        aEncoderCCtx = avcodec_alloc_context3(aEncoderC);
        if (!aEncoderCCtx) {
            std::cout << "Error allocating audio encoder context" << std::endl;
            return -4;
        }

        outAudioStream = avformat_new_stream(outFormatCtx, aEncoderC);
        if (!outAudioStream) {
            std::cout << "Couldn't create output audio AVStream" << std::endl;
            return -1;
        }

        aEncoderCCtx->channels = inAudioStream->codecpar->channels;
        aEncoderCCtx->channel_layout = av_get_default_channel_layout(inAudioStream->codecpar->channels);
        aEncoderCCtx->sample_rate = inAudioStream->codecpar->sample_rate;
        aEncoderCCtx->sample_fmt = aEncoderC->sample_fmts[0];  //for aac , there is AV_SAMPLE_FMT_FLTP =8
        aEncoderCCtx->bit_rate = 32000;
        aEncoderCCtx->time_base.num = 1;
        aEncoderCCtx->time_base.den = aEncoderCCtx->sample_rate;


        if (avcodec_open2(aEncoderCCtx, aEncoderC, nullptr) < 0) {
            std::cout << "Could not open the audio encoder." << std::endl;
            vDecoderCCtx = nullptr;
            return -5;
        }
    }

    return 0;
}

int ScreenRecorder::openOutput() {
    /* Some container formats (like MP4) require global headers to be present
 Mark the encoder so that it behaves accordingly. */
    if ( outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        vEncoderCCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    avcodec_parameters_from_context(outVideoStream->codecpar, vEncoderCCtx);
    av_dump_format(outFormatCtx, videoIndex, outFilename, 1);

    if(true){
        avcodec_parameters_from_context(outAudioStream->codecpar, aEncoderCCtx);
        av_dump_format(outFormatCtx, audioIndex, outFilename, 1);
    }

    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        //avio_open2(&outFormatCtx->pb, filename_out, AVIO_FLAG_WRITE, NULL, NULL);
        if ( avio_open2(&outFormatCtx->pb, outFilename, AVIO_FLAG_WRITE, nullptr, nullptr) < 0) {
            std::cout << "Could not open output file " << outFilename<< std::endl;
            return -1;
        }
    }
    return 0;
}

int ScreenRecorder::writeHeader() {

//    av_dict_set(&muxerOptions, "preset", "fast", 0);
//    av_dict_set(&muxerOptions, "x264-params", "keyint=60:min-keyint=60:scenecut=0", 0);
//    av_dict_set(&muxerOptions, "movflags", "faststart", 0);
//    av_dict_set(&muxerOptions, "brand", "mp42", 0);
//    av_dict_set( &muxerOptions,"framerate","60",0 );

    if (avformat_write_header(outFormatCtx, &muxerOptions) < 0) {
        std::cout << "Failed to write header" << std::endl;
        return -2;
    }



    return 0;
}

int ScreenRecorder::decoding() {

    inFrame = av_frame_alloc();
    if (!inFrame) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        return -1;
    }

    inPacket = av_packet_alloc();
    if (!inPacket) {
        std::cout << "Couldn't allocate AVPacket" << std::endl;
        return -1;
    }
    struct SwsContext *sws_ctx = nullptr;
    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(vDecoderCCtx->width,
                             vDecoderCCtx->height,
                             vDecoderCCtx->pix_fmt,
                             vEncoderCCtx->width,
                             vEncoderCCtx->height,
                             vEncoderCCtx->pix_fmt,
                             SWS_BILINEAR,
                             nullptr,
                             nullptr,
                             nullptr
    );

    const AVSampleFormat requireAudioFmt = AV_SAMPLE_FMT_FLTP;
    SwrContext* swr_ctx;
    swr_ctx = swr_alloc_set_opts(nullptr,
                                 av_get_default_channel_layout(aDecoderCCtx->channels),
                                 requireAudioFmt,  // aac encoder only receive this format
                                 aDecoderCCtx->sample_rate,
                                 av_get_default_channel_layout(aDecoderCCtx->channels),
                                 (AVSampleFormat)inAudioStream->codecpar->format,
                                 inAudioStream->codecpar->sample_rate,
                                 0, nullptr);
    swr_init(swr_ctx);

    audioFifo = av_audio_fifo_alloc(requireAudioFmt, aDecoderCCtx->channels,
                                    aDecoderCCtx->sample_rate * 2);

    int index = 0;
    int nframe = 600;
    i = 0;
    //loop as long we have a frame to read
    while (av_read_frame(videoInFormatCtx, inPacket) >= 0) {
        if(index++ == nframe){
            break;
        }
        if (videoInFormatCtx->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (transcodeVideo(i, sws_ctx)) return -1;
            av_packet_unref(inPacket);
        }else if (audioInFormatCtx->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
                //TODO: transcodeAudio()
        }else {
            std::cout << "ignoring all non video or audio packets" << std::endl;
        }
    }
    return 0;
}

int ScreenRecorder::transcodeVideo(int indexFrame, SwsContext *pContext) {
    /* JUST CHECKING VIDEO! NEED TO MODIFY FOR AUDIO
    if(inPacket->stream_index != inVideoStream->index) continue;
*/
//    av_packet_rescale_ts(inPacket, inVideoStream->time_base, vDecoderCCtx->time_base);

    int res = avcodec_send_packet(vDecoderCCtx, inPacket);
    if(res<0){
        std::cout << "An error happened during the decoding phase" <<std::endl;
        return res;
    }

    while (res >= 0) {
        res = avcodec_receive_frame(vDecoderCCtx, inFrame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0 ){
            std::cout << "Error during encoding" << std::endl;
            return res;
        }

        convFrame = av_frame_alloc();
        if (!convFrame) {
            std::cout << "Couldn't allocate AVFrame" << std::endl;
            return -1;
        }

        uint8_t *buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(vEncoderCCtx->pix_fmt, vEncoderCCtx->width,vEncoderCCtx->height, 1));
        if(buffer==NULL){
            std::cout << "unable to allocate memory for the buffer" << std::endl;
            return -1;
        }
        if((av_image_fill_arrays(convFrame->data, convFrame->linesize, buffer, vEncoderCCtx->pix_fmt, vEncoderCCtx->width,
                                 vEncoderCCtx->height, 1)) <0){
            std::cout << "An error occured while filling the image array" << std::endl;
            return -1;
        };

        convFrame->width = vEncoderCCtx->width;
        convFrame->height = vEncoderCCtx->height;
        convFrame->format = vEncoderCCtx->pix_fmt;
        convFrame->pts = inFrame->pts;

//        convFrame->pts = av_rescale_q(convFrame->pts, vDecoderCCtx->time_base, vEncoderCCtx->time_base);//inFrame->pts; //((1.0/25) * 60 * indexFrame);//indexFrame;//av_rescale_q(inFrame->pts, vEncoderCCtx->time_base, outVideoStream->time_base);//inFrame->pts;
//        av_frame_get_buffer(convFrame, 0);

        sws_scale(pContext, (uint8_t const *const *) inFrame->data,
                  inFrame->linesize, 0, vDecoderCCtx->height,
                  convFrame->data, convFrame->linesize);

        if (res >= 0) {
            if (encode(indexFrame, videoIndex, vEncoderCCtx, outVideoStream)) return -1;
        }
        av_frame_unref(inFrame);

    }
    return 0;
}

int ScreenRecorder::transcodeAudio(int indexFrame, SwrContext *pContext) {

    int res = avcodec_send_packet(aDecoderCCtx, inPacket);
    if(res<0){
        std::cout << "An error happened during the decoding phase" <<std::endl;
        return res;
    }

    while (res >= 0) {
        res = avcodec_receive_frame(aDecoderCCtx, inFrame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0 ){
            std::cout << "Error during encoding" << std::endl;
            return res;
        }

        convFrame = av_frame_alloc();
        if (!convFrame) {
            std::cout << "Couldn't allocate AVFrame" << std::endl;
            return -1;
        }

        uint8_t *buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(vEncoderCCtx->pix_fmt, vEncoderCCtx->width,vEncoderCCtx->height, 1));
        if(buffer==NULL){
            std::cout << "unable to allocate memory for the buffer" << std::endl;
            return -1;
        }
        if((av_image_fill_arrays(convFrame->data, convFrame->linesize, buffer, vEncoderCCtx->pix_fmt, vEncoderCCtx->width,
                                 vEncoderCCtx->height, 1)) <0){
            std::cout << "An error occured while filling the image array" << std::endl;
            return -1;
        };

        convFrame->width = vEncoderCCtx->width;
        convFrame->height = vEncoderCCtx->height;
        convFrame->format = vEncoderCCtx->pix_fmt;
        convFrame->pts = inFrame->pts;

//        convFrame->pts = av_rescale_q(convFrame->pts, vDecoderCCtx->time_base, vEncoderCCtx->time_base);//inFrame->pts; //((1.0/25) * 60 * indexFrame);//indexFrame;//av_rescale_q(inFrame->pts, vEncoderCCtx->time_base, outVideoStream->time_base);//inFrame->pts;
//        av_frame_get_buffer(convFrame, 0);

        sws_scale(pContext, (uint8_t const *const *) inFrame->data,
                  inFrame->linesize, 0, vDecoderCCtx->height,
                  convFrame->data, convFrame->linesize);

        if (res >= 0) {
            if (encode(indexFrame, videoIndex, vEncoderCCtx, outVideoStream)) return -1;
        }
        av_frame_unref(inFrame);

    }
    return 0;

}


int ScreenRecorder::encode(int i, int streamIndex, AVCodecContext* cctx, AVStream* outStream) {
    outPacket = av_packet_alloc();
    if (!outPacket) {
        std::cout << "could not allocate memory for output packet" << std::endl;
        return -1;
    }
    int  res = avcodec_send_frame(vEncoderCCtx, convFrame);
    if (res < 0) {
        std::cout << "Error sending a frame for encoding" << std::endl;
        return -1;
    }

    while (res >= 0){
        res = avcodec_receive_packet(vEncoderCCtx, outPacket);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0) {
            std::cout << "Error during encoding" << std::endl;
            return -1;
        }

        outPacket->stream_index = videoIndex;

        outPacket->pts = i;
        outPacket->dts = i-20;
        outPacket->duration = 60;
        i += 60; //inizia da 1 secondo a visualizzare, ma il video , vautare se aggiornarlo dopo
//        outPacket->duration = outVideoStream->time_base.den / outVideoStream->time_base.num / inVideoStream->avg_frame_rate.num * inVideoStream->avg_frame_rate.den;
        av_packet_rescale_ts(outPacket, vEncoderCCtx->time_base, outVideoStream->time_base);

//        outPacket->dts = av_rescale_q(outPacket->dts, outVideoStream->time_base, inVideoStream->time_base);
//        outPacket->pts = av_rescale_q(outPacket->pts, outVideoStream->time_base, inVideoStream->time_base);

//        outPacket->duration = av_rescale_q(outPacket->duration, vDecoderCCtx->time_base, vEncoderCCtx->time_base );

//        outPacket->pts = av_rescale_q(i, AV_TIME_BASE_Q, outVideoStream->time_base);
//        outPacket->dts = av_rescale_q(outPacket->dts, vEncoderCCtx->time_base, vDecoderCCtx->time_base);
/*
        if (vEncoderCCtx->coded_frame->pts != AV_NOPTS_VALUE)
        outPacket->pts= av_rescale_q(vEncoderCCtx->coded_frame->pts, vEncoderCCtx->time_base, inVideoStream->time_base);
        if(outPacket->dts != AV_NOPTS_VALUE)
            outPacket->dts = av_rescale_q(outPacket->dts, vEncoderCCtx->time_base, vDecoderCCtx->time_base);
*/
//        outPacket->dts = av_rescale_q(outPacket->dts, vEncoderCCtx->time_base, vDecoderCCtx->time_base);

        if(vEncoderCCtx->coded_frame->key_frame)
        outPacket->flags |= AV_PKT_FLAG_KEY;
        std::cout << "before write frame: " << outPacket->pts << " " << outPacket->dts << " duration " << outPacket->duration << res << std::endl;

        res = av_interleaved_write_frame(outFormatCtx, outPacket);
        if (res != 0) {
            std::cout << "Error while writing video frame, error: " << res << std::endl;
            return -1;
        }else{
            std::cout << "writed frame: "  << outPacket->pts << outPacket->dts << res << std::endl;
        }
    }
    av_packet_unref(outPacket);
    av_packet_free(&outPacket);
    return 0;
}

int ScreenRecorder::writeTrailer() {
    av_write_trailer(outFormatCtx);
    if (!(oft->flags & AVFMT_NOFILE)) {
        int err = avio_close(outFormatCtx->pb);
        if (err < 0) {
            std::cout << "Failed to close file" << std::endl;
            return -1;
        }
    }
    return 0;
}

void ScreenRecorder::flushAll() {
    if (muxerOptions != NULL) {
        av_dict_free(&muxerOptions);
        muxerOptions = NULL;
    }

    if (inFrame != NULL) {
        av_frame_free(&inFrame);
        inFrame = NULL;
    }

    if (inPacket != NULL) {
        av_packet_free(&inPacket);
        inPacket = NULL;
    }

    avformat_close_input(&videoInFormatCtx);
    avformat_close_input(&audioInFormatCtx);
}



