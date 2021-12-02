#include "ScreenRecorder.h"

ScreenRecorder::ScreenRecorder() {
    avformat_network_init();
    avdevice_register_all();

    vpts = 0;
    vdts = 0;
    apts = 0;
    last_pts = 0;
    last_apts = 0;

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
//    av_dict_set(&options,"framerate","60",0);
//    av_dict_set(&options,"video_size","wxga",0);
//    av_dict_set(&options, "select_region", "1", 0);
//    av_dict_set(&options, "follow_mouse", "centered", 0);
//    av_dict_set(&options, "draw_mouse", "1", 0);
    av_dict_set(&options, "show_region", "1", 0);
//    videoInFormatCtx->probesize = 40000000;

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
    av_opt_set(vEncoderCCtx->priv_data, "preset", "fast", 0);
    av_opt_set(vEncoderCCtx->priv_data, "x264-params","keyint=250:min-keyint=60:level=4.1:fps=60:crf=1", 0);

    vEncoderCCtx->time_base = (AVRational){1, 30};
    //vEncoderCCtx->framerate = (AVRational){60, 1};
    vEncoderCCtx->width = vDecoderCCtx->width;
    vEncoderCCtx->height = vDecoderCCtx->height;
    vEncoderCCtx->pix_fmt  = AV_PIX_FMT_YUV420P;
    vEncoderCCtx->codec_id = AV_CODEC_ID_H264;
    vEncoderCCtx->codec_type = AVMEDIA_TYPE_VIDEO;
//    vEncoderCCtx->gop_size = 1;
//    vEncoderCCtx->bit_rate = 4000;
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
        aEncoderCCtx->sample_fmt = aEncoderC->sample_fmts[0];  //for aac , there is AV_SAMPLE_FMT_FLTP = 8
        aEncoderCCtx->bit_rate = 128000;
        aEncoderCCtx->time_base.num = 1;
        aEncoderCCtx->time_base.den = aEncoderCCtx->sample_rate;

//        outAudioStream->time_base = aEncoderCCtx->time_base;

        if (avcodec_open2(aEncoderCCtx, aEncoderC, nullptr) < 0) {
            std::cout << "Could not open the audio encoder." << std::endl;
            vDecoderCCtx = nullptr;
            return -1;
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


//    transcodeVideo(sws_ctx);
//    transcodeAudio(swr_ctx);
    //START THREADS
    std::thread threadVideo(&ScreenRecorder::transcodeVideo, this, sws_ctx);

    std::thread threadAudio(&ScreenRecorder::transcodeAudio, this, swr_ctx);
    threadVideo.join();
    threadAudio.join();
//    std::this_thread::sleep_for(std::chrono::seconds(20));

    return 0;
}

int ScreenRecorder::transcodeVideo(SwsContext *pContext) {
int i = 0;
    inVideoFrame = av_frame_alloc();
    if (!inVideoFrame) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        return -1;
    }

    inVideoPacket = av_packet_alloc();
    if (!inVideoFrame) {
        std::cout << "Couldn't allocate AVPacket" << std::endl;
        return -1;
    }

    while (av_read_frame(videoInFormatCtx, inVideoPacket) >= 0 && i++<1000) {


    int res = avcodec_send_packet(vDecoderCCtx, inVideoPacket);
    if(res<0){
        std::cout << "An error happened during the decoding phase" <<std::endl;
        return res;
    }

    while (res >= 0) {
        res = avcodec_receive_frame(vDecoderCCtx, inVideoFrame);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0) {
            std::cout << "Error during encoding" << std::endl;
            return res;
        }

        convVideoFrame = av_frame_alloc();
        if (!convVideoFrame) {
            std::cout << "Couldn't allocate AVFrame" << std::endl;
            return -1;
        }

        uint8_t *buffer = (uint8_t *) av_malloc(
                av_image_get_buffer_size(vEncoderCCtx->pix_fmt, vEncoderCCtx->width, vEncoderCCtx->height, 1));
        if (buffer == NULL) {
            std::cout << "unable to allocate memory for the buffer" << std::endl;
            return -1;
        }
        if ((av_image_fill_arrays(convVideoFrame->data, convVideoFrame->linesize, buffer, vEncoderCCtx->pix_fmt,
                                  vEncoderCCtx->width,
                                  vEncoderCCtx->height, 1)) < 0) {
            std::cout << "An error occured while filling the image array" << std::endl;
            return -1;
        };

        convVideoFrame->width = vEncoderCCtx->width;
        convVideoFrame->height = vEncoderCCtx->height;
        convVideoFrame->format = vEncoderCCtx->pix_fmt;

        sws_scale(pContext, (uint8_t const *const *) inVideoFrame->data,
                  inVideoFrame->linesize, 0, vDecoderCCtx->height,
                  convVideoFrame->data, convVideoFrame->linesize);

        convVideoFrame->pts = vpts * outVideoStream->time_base.den * 1 / vEncoderCCtx->time_base.den;

        if (res >= 0) {
            outVideoPacket = av_packet_alloc();
            if (!outVideoPacket) {
                std::cout << "could not allocate memory for output packet" << std::endl;
                return -1;
            }
            int  res = avcodec_send_frame(vEncoderCCtx, convVideoFrame);
            if (res < 0) {
                std::cout << "Error sending a frame for encoding" << std::endl;
                return -1;
            }

            while (res >= 0) {
                res = avcodec_receive_packet(vEncoderCCtx, outVideoPacket);
                if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                    break;
                } else if (res < 0) {
                    std::cout << "Error during encoding" << std::endl;
                    return -1;
                }

                outVideoPacket->stream_index = 0;
                outVideoPacket->duration = outVideoStream->time_base.den * 1 / vEncoderCCtx->time_base.den;
                outVideoPacket->dts = outVideoPacket->pts = vpts * outVideoStream->time_base.den * 1 / vEncoderCCtx->time_base.den;
                vpts++;
                if(vEncoderCCtx->coded_frame->key_frame)
                    outVideoPacket->flags |= AV_PKT_FLAG_KEY;

                std::cout << "before write frame: " << outVideoPacket->pts << " " << outVideoPacket->dts << " duration " << outVideoPacket->duration << " streamindex: " << outVideoPacket->stream_index << std::endl;

//                std::lock_guard<std::mutex> l(mutexWriteFrame);
                res = av_interleaved_write_frame(outFormatCtx, outVideoPacket);

                if (res != 0) {
                    std::cout << "Error while writing video frame, error: " << res << std::endl;
                    return -1;
                }
            }
            av_packet_unref(outVideoPacket);
            av_packet_free(&outVideoPacket);
        }
        av_frame_unref(inVideoFrame);
    }
        av_packet_unref(inVideoPacket);
}
    return 0;
}

int ScreenRecorder::transcodeAudio(SwrContext *pContext) {
int i = 0;
    inAudioFrame = av_frame_alloc();
    if (!inAudioFrame) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        return -1;
    }

    inAudioPacket = av_packet_alloc();
    if (!inAudioPacket) {
        std::cout << "Couldn't allocate AVPacket" << std::endl;
        return -1;
    }

    while (av_read_frame(audioInFormatCtx, inAudioPacket) >= 0 && i++<2000) {


    int res = avcodec_send_packet(aDecoderCCtx, inAudioPacket);
        if(res<0){
            std::cout << "An error happened during the decoding phase" <<std::endl;
            return res;
        }

        res = avcodec_receive_frame(aDecoderCCtx, inAudioFrame);
        if (res < 0 ){
            std::cout << "Error during encoding" << std::endl;
            return res;
        }

        uint8_t **cSamples = nullptr;
        res = av_samples_alloc_array_and_samples(&cSamples, NULL, aEncoderCCtx->channels, inAudioFrame->nb_samples, requireAudioFmt, 0);
        if (res < 0) {
             std::cout << "Fail to alloc samples by av_samples_alloc_array_and_samples." << std::endl;
        }
        res = swr_convert(pContext, cSamples, inAudioFrame->nb_samples, (const uint8_t**)inAudioFrame->extended_data, inAudioFrame->nb_samples);
        if (res < 0) {
            std::cout << "Failed swr_convert." << std::endl;
        }
        if (av_audio_fifo_space(audioFifo) < inAudioFrame->nb_samples) {
            std::cout << "audio buffer is too small." << std::endl;
        }

        res = av_audio_fifo_write(audioFifo, (void**)cSamples, inAudioFrame->nb_samples);
        if (res < 0) {
            throw std::runtime_error("Fail to write fifo");
        }

        av_freep(&cSamples[0]);

        while (av_audio_fifo_size(audioFifo) >= aEncoderCCtx->frame_size) {
            convAudioFrame = av_frame_alloc();
            if (!convAudioFrame) {
                std::cout << "Couldn't allocate AVFrame" << std::endl;
                return -1;
            }
            convAudioFrame->nb_samples = aEncoderCCtx->frame_size;
            convAudioFrame->channels = aDecoderCCtx->channels;
            convAudioFrame->channel_layout = av_get_default_channel_layout(aDecoderCCtx->channels);
            convAudioFrame->format = requireAudioFmt;
            convAudioFrame->sample_rate = aEncoderCCtx->sample_rate;


            res = av_frame_get_buffer(convAudioFrame, 0);
            res = av_audio_fifo_read(audioFifo, (void**)convAudioFrame->data, aEncoderCCtx->frame_size);

            convAudioFrame->pts = apts * outAudioStream->time_base.den * 1024 / aEncoderCCtx->sample_rate; //inAudioFrame->pts;

            outAudioPacket = av_packet_alloc();
            if (!outAudioPacket) {
                std::cout << "could not allocate memory for output packet" << std::endl;
                return -1;
            }
            int  res = avcodec_send_frame(aEncoderCCtx, convAudioFrame);
            if (res < 0) {
                std::cout << "Error sending a frame for encoding" << std::endl;
                return -1;
            }

                res = avcodec_receive_packet(aEncoderCCtx, outAudioPacket);
                if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                    break;
                } else if (res < 0) {
                    std::cout << "Error during encoding" << std::endl;
                    return -1;
                }

                outAudioPacket->stream_index = 1;
                outAudioPacket->duration = outAudioStream->time_base.den * 1024 / aEncoderCCtx->sample_rate;
                outAudioPacket->dts = outAudioPacket->pts = apts * outAudioStream->time_base.den * 1024 / aEncoderCCtx->sample_rate;
                apts++;

                std::cout << "before write frame: " << outAudioPacket->pts << " " << outAudioPacket->dts << " duration " << outAudioPacket->duration << " streamindex: " << 1 << std::endl;

//                std::lock_guard<std::mutex> l(mutexWriteFrame);
                res = av_interleaved_write_frame(outFormatCtx, outAudioPacket);
                if (res != 0) {
                    std::cout << "Error while writing video frame, error: " << res << std::endl;
                    return -1;
                }
            av_packet_unref(outAudioPacket);
            av_packet_free(&outAudioPacket);
        }
        av_frame_unref(inAudioFrame);
        av_packet_unref(inAudioPacket);
    }
    return 0;

}


int ScreenRecorder::encode(int streamIndex, AVCodecContext* cctx, AVStream* outStream, AVFrame *convframe) {
//    outPacket = av_packet_alloc();
//    if (!outPacket) {
//        std::cout << "could not allocate memory for output packet" << std::endl;
//        return -1;
//    }
//    int  res = avcodec_send_frame(cctx, convframe);
//    if (res < 0) {
//        std::cout << "Error sending a frame for encoding" << std::endl;
//        return -1;
//    }
//
//    while (res >= 0) {
//        res = avcodec_receive_packet(cctx, outPacket);
//        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
//            break;
//        } else if (res < 0) {
//            std::cout << "Error during encoding" << std::endl;
//            return -1;
//        }
//
//        outPacket->stream_index = streamIndex;
//        if(streamIndex == 0) {
//
//            outPacket->duration = outVideoStream->time_base.den * 1 / vEncoderCCtx->time_base.den;
//            outPacket->dts = outPacket->pts = vpts * outVideoStream->time_base.den * 1 / vEncoderCCtx->time_base.den;
//            vpts++;
//            if(vEncoderCCtx->coded_frame->key_frame)
//                outPacket->flags |= AV_PKT_FLAG_KEY;
//        }else {
//            outPacket->duration = outAudioStream->time_base.den * 1024 / aEncoderCCtx->sample_rate;
//            outPacket->dts = outPacket->pts = apts * outAudioStream->time_base.den * 1024 / aEncoderCCtx->sample_rate;
//            apts++;
//        }
//
//        std::cout << "before write frame: " << outPacket->pts << " " << outPacket->dts << " duration " << outPacket->duration << " streamindex: " << streamIndex <<res << std::endl;
//
//    std::lock_guard<std::mutex> l(mutexWriteFrame);
//    res = av_interleaved_write_frame(outFormatCtx, outPacket);
//    if (res != 0) {
//        std::cout << "Error while writing video frame, error: " << res << std::endl;
//        return -1;
//    }
//
//    if(streamIndex == 1)
//        break;
//
//    }
//    av_packet_unref(outPacket);
//    av_packet_free(&outPacket);
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


    avformat_close_input(&videoInFormatCtx);
    avformat_close_input(&audioInFormatCtx);
}



