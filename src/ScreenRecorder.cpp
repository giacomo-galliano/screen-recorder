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
    oft = av_guess_format(nullptr, outFilename, nullptr);
    if(!oft){
        std::cout << "Can't create output format" << std::endl;
        return -1;
    }
    avformat_alloc_output_context2(&outFormatCtx, oft, NULL, outFilename);

    encoderC = avcodec_find_encoder((AV_CODEC_ID_H264));
    if(!encoderC){
        std::cout << "An error occurred trying to find the encoder codec" << std::endl;
        return -3;
    }

    outVideoStream = avformat_new_stream(outFormatCtx, encoderC);
    if (!outFormatCtx) {
        std::cout << "Couldn't create output AVFormatContext" << std::endl;
        return -2;
    }

    encoderCCtx = avcodec_alloc_context3(encoderC);
    if (!encoderCCtx) {
        std::cout << "Error allocating encoder context" << std::endl;
        return -4;
    }

    outVideoStream->time_base = (AVRational){25, 1};
//    encoderCCtx->sample_fmt = encoderC->sample_fmts ? encoderC->sample_fmts[0] : AV_SAMPLE_FMT_S16;
    encoderCCtx->time_base = outVideoStream->time_base;
    encoderCCtx->width = decoderCCtx->width;
    encoderCCtx->height = decoderCCtx->height;
    encoderCCtx->pix_fmt  = AV_PIX_FMT_YUV420P;
    encoderCCtx->codec_id = AV_CODEC_ID_H264;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
    encoderCCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    encoderCCtx->gop_size = 12;
//    encoderCCtx->level = 31;


    av_opt_set(encoderCCtx->priv_data, "crf", "12", 0);
//    av_opt_set(encoderCCtx->priv_data, "profile", "main", 0);
    av_opt_set(encoderCCtx->priv_data, "preset", "medium", 0);
//    av_opt_set(encoderCCtx->priv_data, "b-pyramid", "0", 0);

    /*default setings x264*/
//    encoderCCtx->me_range = 16;
//    encoderCCtx->max_qdiff = 4;
//    encoderCCtx->qmin = 10;
//    encoderCCtx->qmax = 51;
//    encoderCCtx->qcompress = 0.6;


//    encoderCCtx->codec_id = AV_CODEC_ID_H264;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
//    encoderCCtx->codec_type = AVMEDIA_TYPE_VIDEO;
//    encoderCCtx->pix_fmt  = AV_PIX_FMT_YUV420P;
//    encoderCCtx->bit_rate = 2000000;
//////    encoderCCtx->rc_buffer_size = 4000000;
//////    encoderCCtx->rc_max_rate = 2000000;
//////    encoderCCtx->rc_min_rate = 2000000;
//    encoderCCtx->width = decoderCCtx->width;
//    encoderCCtx->height = decoderCCtx->height;
//////    encoderCCtx->sample_aspect_ratio = decoderCCtx->sample_aspect_ratio;
//    encoderCCtx->gop_size = 40;
//    encoderCCtx->max_b_frames = 3;
////
//////    encoderCCtx->time_base = av_inv_q(av_guess_frame_rate(inFormatCtx, inVideoStream, NULL));
//    encoderCCtx->time_base = (AVRational){1, 30};
//////    encoderCCtx->time_base.num = 1;
//////    encoderCCtx->time_base.den = 30; // 30->15fps
////    encoderCCtx->framerate = (AVRational){25, 1};
//
////    encoderCCtx->time_base = decoderCCtx -> time_base;
//    outVideoStream->time_base = encoderCCtx->time_base;


//    encoderCCtx->bit_rate = 500*1000;
//    encoderCCtx->bit_rate_tolerance = 0;
//    encoderCCtx->rc_max_rate = 0;
//    encoderCCtx->rc_buffer_size = 0;
//    encoderCCtx->gop_size = 40;
//    encoderCCtx->max_b_frames = 3;
//    encoderCCtx->b_frame_strategy = 1;
//    encoderCCtx->coder_type = 1;
//    encoderCCtx->me_cmp = 1;
//    encoderCCtx->me_range = 16;
//    encoderCCtx->qmin = 10;
//    encoderCCtx->qmax = 51;
//    encoderCCtx->scenechange_threshold = 40;
//    encoderCCtx->flags |= AV_CODEC_FLAG_LOOP_FILTER;
//    encoderCCtx->me_subpel_quality = 5;
//    encoderCCtx->i_quant_factor = 0.71;
//    encoderCCtx->qcompress = 0.6;
//    encoderCCtx->max_qdiff = 4;
//    encoderCCtx->flags2 |= AV_CODEC_FLAG2_FAST;

    if (avcodec_open2(encoderCCtx, encoderC, nullptr) < 0) {
        std::cout << "Could not open the encoder." << std::endl;
        decoderCCtx = nullptr;
        return -5;
    }
    avcodec_parameters_from_context(outVideoStream->codecpar, encoderCCtx);
    return 0;
}

int ScreenRecorder::openOutput() {
    /* Some container formats (like MP4) require global headers to be present
 Mark the encoder so that it behaves accordingly. */
//    if ( outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
//    {
//        encoderCCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//    }

    av_dump_format(outFormatCtx, 0, outFilename, 1);

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
    sws_ctx = sws_getContext(decoderCCtx->width,
                             decoderCCtx->height,
                             decoderCCtx->pix_fmt,
                             encoderCCtx->width,
                             encoderCCtx->height,
                             encoderCCtx->pix_fmt,
                             SWS_BILINEAR,
                             nullptr,
                             nullptr,
                             nullptr
    );
    int index = 0;
    int nframe = 200;
    //loop as long we have a frame to read
    while (av_read_frame(inFormatCtx, inPacket) >= 0) {
        if(index++ == nframe){
            break;
        }
        if (inFormatCtx->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (transcodeVideo(index, sws_ctx)) return -1;
            av_packet_unref(inPacket);
        }else if (inFormatCtx->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
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
    int res = avcodec_send_packet(decoderCCtx, inPacket);
    if(res<0){
        std::cout << "An error happened during the decoding phase" <<std::endl;
        return res;
    }

    while (res >= 0) {
        res = avcodec_receive_frame(decoderCCtx, inFrame);
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

        uint8_t *buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(encoderCCtx->pix_fmt, encoderCCtx->width,encoderCCtx->height, 1));
        if(buffer==NULL){
            std::cout << "unable to allocate memory for the buffer" << std::endl;
            return -1;
        }
        if((av_image_fill_arrays(convFrame->data, convFrame->linesize, buffer, encoderCCtx->pix_fmt, encoderCCtx->width,
                                 encoderCCtx->height, 1)) <0){
            std::cout << "An error occured while filling the image array" << std::endl;
            return -1;
        };

        convFrame->width = encoderCCtx->width;
        convFrame->height = encoderCCtx->height;
        convFrame->format = encoderCCtx->pix_fmt;
        convFrame->pts = indexFrame;//av_rescale_q(inFrame->pts, encoderCCtx->time_base, outVideoStream->time_base);//((1.0/25) * 60 * indexFrame);//inFrame->pts;
//        av_frame_get_buffer(convFrame, 0);

        sws_scale(pContext, (uint8_t const *const *) inFrame->data,
                  inFrame->linesize, 0, decoderCCtx->height,
                  convFrame->data, convFrame->linesize);

        if (res >= 0) {
            if (encodeVideo(indexFrame)) return -1;
        }
        av_frame_unref(inFrame);

    }
    return 0;
}

int ScreenRecorder::encodeVideo(int i) {
    outPacket = av_packet_alloc();
    if (!outPacket) {
        std::cout << "could not allocate memory for output packet" << std::endl;
        return -1;
    }
    int  res = avcodec_send_frame(encoderCCtx, convFrame);
    if (res < 0) {
        std::cout << "Error sending a frame for encoding" << std::endl;
        return -1;
    }

    while (res >= 0){
        res = avcodec_receive_packet(encoderCCtx, outPacket);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            break;
        } else if (res < 0) {
            std::cout << "Error during encoding" << std::endl;
            return -1;
        }

        outPacket->stream_index = videoIndex;
        outPacket->pts = i*100;
//        outPacket->dts = i;
//        outPacket->duration = 1;//outVideoStream->time_base.den / outVideoStream->time_base.num / inVideoStream->avg_frame_rate.num * inVideoStream->avg_frame_rate.den;
/*
        if (encoderCCtx->coded_frame->pts != AV_NOPTS_VALUE)
        outPacket->pts= av_rescale_q(encoderCCtx->coded_frame->pts, encoderCCtx->time_base, inVideoStream->time_base);
        if(outPacket->dts != AV_NOPTS_VALUE)
            outPacket->dts = av_rescale_q(outPacket->dts, encoderCCtx->time_base, decoderCCtx->time_base);
*/
//        if(encoderCCtx->coded_frame->key_frame)
            outPacket->flags |= AV_PKT_FLAG_KEY;

            outPacket->duration = 1;


//        av_packet_rescale_ts(outPacket, decoderCCtx->time_base, encoderCCtx->time_base);
//        outPacket->dts = av_rescale_q(outPacket->dts, encoderCCtx->time_base, decoderCCtx->time_base);

        std::cout << "before write frame: " << outPacket->flags << " " <<outPacket->pts << " " << outPacket->dts << " duration " << outPacket->duration << res << std::endl;

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

    avformat_close_input(&inFormatCtx);

}