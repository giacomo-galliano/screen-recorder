#include "ScreenRecorder.h"

ScreenRecorder::ScreenRecorder() {
    avformat_network_init();
    avdevice_register_all();

    vpts = 0;
    vdts = 0;
    apts = 0;
    last_pts = 0;
    last_dts = 0;

    outFilename = "../media/output.mp4";
    muxerOptions = nullptr;
}

ScreenRecorder::~ScreenRecorder() {

}

int ScreenRecorder::fillStreamInfo() {
    /***VIDEO-INFO***/
    vDecoderC = avcodec_find_decoder(videoInFormatCtx->streams[videoIndex]->codecpar->codec_id);
    if (!vDecoderC) {
        std::cout << "failed to find the video codec." << std::endl;
        return -1;
    }

    vDecoderCCtx = avcodec_alloc_context3(vDecoderC);
    if (!vDecoderCCtx) {
        std::cout << "Error allocating video decoder context" << std::endl;
        avformat_close_input(&videoInFormatCtx);
        return -1;
    }

    if(avcodec_parameters_to_context(vDecoderCCtx, videoInFormatCtx->streams[videoIndex]->codecpar)){
        avformat_close_input(&videoInFormatCtx);
        avcodec_free_context(&vDecoderCCtx);
        return -2;
    }

    //// BISOGNA SETTARLO???? DI DEFAULT E' 0/1.
    vDecoderCCtx->time_base = videoInFormatCtx->streams[videoIndex]->time_base; // (AVRational){1, 90000};

    if (avcodec_open2(vDecoderCCtx, vDecoderC, nullptr) < 0) {
        std::cout << "failed to open video decoder." << std::endl;
        vDecoderCCtx = nullptr;
        return -3;
    }

    /***AUDIO-INFO***/
    aDecoderC = avcodec_find_decoder(audioInFormatCtx->streams[audioIndex]->codecpar->codec_id);
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

    if(avcodec_parameters_to_context(aDecoderCCtx, audioInFormatCtx->streams[audioIndex]->codecpar)){
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
        if (videoInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
        }
    }
    if(true) { //da controllare lo status
        for (int i = 0; i < audioInFormatCtx->nb_streams; i++) {
            if (audioInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
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

    videoInFormatCtx = avformat_alloc_context();
    if (!videoInFormatCtx) {
        std::cout << "Couldn't create input AVFormatContext" << std::endl;
        return(-1);
    }

    audioInFormatCtx = avformat_alloc_context();
    if (!audioInFormatCtx) {
        std::cout << "Couldn't create input AVFormatContext" << std::endl;
        return(-1);
    }

    vIft = av_find_input_format("x11grab");
    aIft = av_find_input_format("pulse");
    AVDictionary* options = NULL;
//    av_dict_set(&options,"framerate","10000",0);
//    av_dict_set(&options,"video_size","wxga",0);
//    av_dict_set(&options, "crf", "12", 0);
    videoInFormatCtx->probesize = 40000000;

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
    //vEncoderCCtx->framerate = (AVRational){60, 1};
    vEncoderCCtx->width = vDecoderCCtx->width;
    vEncoderCCtx->height = vDecoderCCtx->height;
    vEncoderCCtx->pix_fmt  = AV_PIX_FMT_YUV420P;
    vEncoderCCtx->codec_id = AV_CODEC_ID_H264;
    vEncoderCCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    vEncoderCCtx->gop_size = 24;
    vEncoderCCtx->bit_rate = 4000000;
//    vEncoderCCtx->level = 31;
    vEncoderCCtx->framerate = av_inv_q(vEncoderCCtx->time_base);

    //outVideoStream->time_base = vEncoderCCtx->time_base;


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

        aEncoderCCtx->channels = audioInFormatCtx->streams[audioIndex]->codecpar->channels;
        aEncoderCCtx->channel_layout = av_get_default_channel_layout(audioInFormatCtx->streams[audioIndex]->codecpar->channels);
        aEncoderCCtx->sample_rate = audioInFormatCtx->streams[audioIndex]->codecpar->sample_rate;
        aEncoderCCtx->sample_fmt = aEncoderC->sample_fmts[0];  //for aac , there is AV_SAMPLE_FMT_FLTP =8
        aEncoderCCtx->bit_rate = 32000;
        aEncoderCCtx->time_base.num = 1;
        aEncoderCCtx->time_base.den = aEncoderCCtx->sample_rate;

        outAudioStream->time_base = aEncoderCCtx->time_base;

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

    if (avformat_write_header(outFormatCtx, nullptr) < 0) {
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

    SwrContext* swr_ctx;
    swr_ctx = swr_alloc_set_opts(nullptr,
                                 av_get_default_channel_layout(aDecoderCCtx->channels),
                                 requireAudioFmt,  // aac encoder only receive this format
                                 aDecoderCCtx->sample_rate,
                                 av_get_default_channel_layout(aDecoderCCtx->channels),
                                 (AVSampleFormat)audioInFormatCtx->streams[audioIndex]->codecpar->format,
                                 audioInFormatCtx->streams[audioIndex]->codecpar->sample_rate,
                                 0, nullptr);
    swr_init(swr_ctx);

    audioFifo = av_audio_fifo_alloc(requireAudioFmt, aDecoderCCtx->channels,
                                    aDecoderCCtx->sample_rate * 4);



    int index = 0;
    int nframe = 200;
    int ai = 0;
    int vi = 0;
    //loop as long we have a frame to read
    while(index++ < nframe){
        if(av_read_frame(videoInFormatCtx, inPacket) >= 0) {

            if (videoInFormatCtx->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                if (transcodeVideo(&vi, sws_ctx)) return -1;
                av_packet_unref(inPacket);
            }

            if (av_read_frame(audioInFormatCtx, inPacket) >= 0 && false) {
                if (audioInFormatCtx->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    if (transcodeAudio(&ai, swr_ctx)) return -1;
                    av_packet_unref(inPacket);
                }
            }
        }
    }
    return 0;
}

int ScreenRecorder::transcodeVideo(int* index, SwsContext *pContext) {

//    AVRational time_base1=videoInFormatCtx->streams[videoIndex]->time_base;
//    //Duration between 2 frames (us)
//    int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(videoInFormatCtx->streams[videoIndex]->r_frame_rate);
//    //Parameters
//    inPacket->pts=(double)(vpts*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
//    inPacket->dts=inPacket->pts;
//    inPacket->duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
//    vpts++;

    av_packet_rescale_ts(inPacket, AV_TIME_BASE_Q, videoInFormatCtx->streams[videoIndex]->time_base);
//    av_packet_rescale_ts(inPacket, videoInFormatCtx->streams[videoIndex]->time_base, FLICKS_TIMESCALE_Q);



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
//        convFrame->pts = inFrame->pts;
//        convFrame->pkt_dts = inFrame->pkt_dts;
//        convFrame->pkt_duration = inFrame->pkt_duration;
//        convFrame->pkt_pos = inFrame->pkt_pos;
//        convFrame->pkt_size = inFrame->pkt_size;

        av_frame_copy_props(convFrame, inFrame);


        sws_scale(pContext, (uint8_t const *const *) inFrame->data,
                  inFrame->linesize, 0, vDecoderCCtx->height,
                  convFrame->data, convFrame->linesize);

        if (res >= 0) {
            if (encode(index, videoIndex, vEncoderCCtx, outVideoStream)) return -1;
        }
        av_frame_unref(inFrame);

    }
    return 0;
}

int ScreenRecorder::transcodeAudio(int* index, SwrContext *pContext) {

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

        uint8_t **cSamples = nullptr;
        res = av_samples_alloc_array_and_samples(&cSamples, NULL, aEncoderCCtx->channels, inFrame->nb_samples, requireAudioFmt, 0);
        if (res < 0) {
             std::cout << "Fail to alloc samples by av_samples_alloc_array_and_samples." << std::endl;
        }
        res = swr_convert(pContext, cSamples, inFrame->nb_samples, (const uint8_t**)inFrame->extended_data, inFrame->nb_samples);
        if (res < 0) {
            std::cout << "Failed swr_convert." << std::endl;
        }
        if (av_audio_fifo_space(audioFifo) < inFrame->nb_samples) {
            std::cout << "audio buffer is too small." << std::endl;
        }

        res = av_audio_fifo_write(audioFifo, (void**)cSamples, inFrame->nb_samples);
        if (res < 0) {
            throw std::runtime_error("Fail to write fifo");
        }

        av_freep(&cSamples[0]);

        while (av_audio_fifo_size(audioFifo) >= aEncoderCCtx->frame_size) {
            convFrame = av_frame_alloc();
            if (!convFrame) {
                std::cout << "Couldn't allocate AVFrame" << std::endl;
                return -1;
            }
            convFrame->nb_samples = aEncoderCCtx->frame_size;
            convFrame->channels = aDecoderCCtx->channels;
            convFrame->channel_layout = av_get_default_channel_layout(aDecoderCCtx->channels);
            convFrame->format = requireAudioFmt;
            convFrame->sample_rate = aEncoderCCtx->sample_rate;

            res = av_frame_get_buffer(convFrame, 0);
            res = av_audio_fifo_read(audioFifo, (void**)convFrame->data, aEncoderCCtx->frame_size);

            convFrame->pts = apts++;
            if (res >= 0) {
                if (encode(index, 1, aEncoderCCtx, outAudioStream)) return -1;
            }
        }

        av_frame_unref(inFrame);

    }
    return 0;

}


int ScreenRecorder::encode(int* i, int streamIndex, AVCodecContext* cctx, AVStream* outStream) {
    outPacket = av_packet_alloc();
    if (!outPacket) {
        std::cout << "could not allocate memory for output packet" << std::endl;
        return -1;
    }
    int  res = avcodec_send_frame(cctx, convFrame);
    if (res < 0) {
        std::cout << "Error sending a frame for encoding" << std::endl;
        return -1;
    }

    while (res >= 0) {
        res = avcodec_receive_packet(cctx, outPacket);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0) {
            std::cout << "Error during encoding" << std::endl;
            return -1;
        }

        outPacket->stream_index = streamIndex;
        if(streamIndex == 0) {
//            auto cts = outPacket->pts - outPacket->dts;
//            auto dts = outPacket->dts;
//            outPacket->dts = vdts + outPacket->duration;
//            vdts = dts;
//            outPacket->pts = outPacket->dts + cts;

//            //Conversion of PTS/DTS Timing
//            outPacket->pts = av_rescale_q_rnd(outPacket->pts, videoInFormatCtx->streams[videoIndex]->time_base, outFormatCtx->streams[videoIndex]->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//            outPacket->dts = av_rescale_q_rnd(outPacket->dts, videoInFormatCtx->streams[videoIndex]->time_base, outFormatCtx->streams[videoIndex]->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//            //printf("pts %d dts %d base %d\n",pkt.pts,pkt.dts, in_stream->time_base);
//            outPacket->duration = av_rescale_q(outPacket->duration, videoInFormatCtx->streams[videoIndex]->time_base, outFormatCtx->streams[videoIndex]->time_base);
//            outPacket->pos = -1;




            //la conversione penso sia giusta, ma non partendo da 0 il video non viene visualizzato, come portare gli inPackets->pts a partire da 0?
            av_packet_rescale_ts(outPacket, videoInFormatCtx->streams[videoIndex]->time_base, outFormatCtx->streams[0]->time_base);
//            av_packet_rescale_ts(outPacket, FLICKS_TIMESCALE_Q, outFormatCtx->streams[0]->time_base);
            if(vpts == 0){
                vpts = outPacket->pts;
                outPacket->pts = 0;
                outPacket->dts = 0;
            }else{
                auto pts = outPacket->pts;
                outPacket->pts = outPacket->pts - vpts + last_pts;
                outPacket->dts = last_pts;
                vpts = pts;
                last_pts = outPacket->pts;
            }

        }else {
            av_packet_rescale_ts(outPacket, cctx->time_base, outStream->time_base);
        }


//    if(streamIndex == 1) {
//        outPacket->pts = *i;
//        outPacket->dts = *i;
//        outPacket->duration = cctx->time_base.den;
//        *i += cctx->time_base.den;
//    }
//        outPacket->duration = outVideoStream->time_base.den / outVideoStream->time_base.num / inVideoStream->avg_frame_rate.num * inVideoStream->avg_frame_rate.den;

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
        std::cout << "before write frame: " << outPacket->pts << " " << outPacket->dts << " duration " << outPacket->duration << " streamindex: " << streamIndex <<res << std::endl;

    res = av_interleaved_write_frame(outFormatCtx, outPacket);
    if (res != 0) {
        std::cout << "Error while writing video frame, error: " << res << std::endl;
        return -1;
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



