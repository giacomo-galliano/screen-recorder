#include "wrappers.h"

void init(){
    avformat_network_init();
    avdevice_register_all();
}

int readFrame(AVFormatContext* fmtCtx, AVPacket* pkt){
    if(!fmtCtx || !pkt){
        return AVERROR(1);
    }
    int err = av_read_frame(fmtCtx, pkt);
    if(err >= 0){
        auto& stream = fmtCtx->streams[pkt->stream_index];
        //av_packet_rescale_ts(pkt, stream->time_base, )
    }else{
        av_init_packet(pkt);
        pkt->size = 0;
        pkt->data = nullptr;
    }
    return err;
}

int writeHeader(FormatContext& fmtCtx){
    if (avformat_write_header(fmtCtx.get(), nullptr) < 0) {
        std::cout << "Failed to write header" << std::endl;
        return -2;
    }
    return 0;
};

int writeTrailer(FormatContext& fmtCtx){
    av_write_trailer(fmtCtx.get());
    //if (!(oft->flags & AVFMT_NOFILE)) {
        int err = avio_close(fmtCtx->pb);
        if (err < 0) {
            std::cout << "Failed to close file" << std::endl;
            return -1;
        }
    //}
    return 0;
};


int prepareDecoder(FormatContext& fmtCtx, AVMediaType mediaType){
    AVCodec* codec = nullptr;
    int index = -1;

    fmtCtx->probesize = 40000000;

    for(int i=0; i < fmtCtx->nb_streams; i++){
        if (fmtCtx->streams[i]->codecpar->codec_type == mediaType){
            index = i;
        }
    }
    if (index == -1) {
        std::cerr << "Media does not contain any usable stream of type " << mediaType << std::endl;
        return -1;
    }

    codec = avcodec_find_decoder(fmtCtx->streams[index]->codecpar->codec_id);
    if(!codec){
        std::cerr << "Failed to find the codec for media type " << mediaType << std::endl;
        return -1;
    }

    CodecContext cCtx = CodecContext (avcodec_alloc_context3(codec), [](AVCodecContext *ctx){
        avcodec_free_context(&ctx);
    });

    if(avcodec_parameters_to_context(cCtx.get(), fmtCtx->streams[index]->codecpar)<0){
        return -1;
    }

    if(avcodec_open2(cCtx.get(), codec, nullptr)<0){
        std::cerr << "Failed to initialize the decoder context for media type " << mediaType << std::endl;
        return -1;
    }

    fmtCtx.open_streams.emplace(index, std::move(cCtx));
    return index;
};

int prepareEncoder(FormatContext* inFmtCtx, FormatContext* outFmtCtx, AVMediaType mediaType){
    AVCodec* codec = nullptr;
    if(mediaType == AVMEDIA_TYPE_VIDEO){
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }else if(mediaType == AVMEDIA_TYPE_AUDIO){
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    }

    if(!codec){
        std::cerr << "Failed to find the ecnoder codec for media type " << mediaType << std::endl;
        return -1;
    }

    CodecContext cCtx = CodecContext (avcodec_alloc_context3(codec), [](AVCodecContext *ctx){
        avcodec_free_context(&ctx);
    });

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        cCtx->codec_id = AV_CODEC_ID_H264;
        cCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        cCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    }else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        cCtx->channels = inFmtCtx->open_streams.find(0)->second.get()->channels;
        cCtx->channel_layout = av_get_default_channel_layout(inFmtCtx->open_streams.find(0)->second.get()->channels);
        cCtx->sample_rate = inFmtCtx->open_streams.find(0)->second.get()->sample_rate;
        cCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
        cCtx->bit_rate = 32000;
        cCtx->time_base.num = 1;
        cCtx->time_base.den = cCtx->sample_rate;
    }

    AVCodecParameters* cp;
    avcodec_parameters_from_context(cp, cCtx.get());
    generateOutStreams(*outFmtCtx, cp, mediaType);

    if(avcodec_open2(cCtx.get(), codec, nullptr)<0){
        std::cerr << "Failed to initialize the decoder context for media type " << mediaType << std::endl;
        return -1;
    }

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        outFmtCtx->open_streams.emplace(0, std::move(cCtx));
    }else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        outFmtCtx->open_streams.emplace(1, std::move(cCtx));
    }

    return 0;
};

void decode(FormatContext& inFmtCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType){
    int index = 0;
    int nframe = 200;
    Packet pkt = Packet(inFmtCtx.get());
    while(index++ < nframe){
        if(readFrame(inFmtCtx.get(), pkt.get())){
            sendPacket(inFmtCtx, outFmtCtx, pkt.get());
        }
    }
    /*
    for(auto& pkt : inFmtCtx) {
        sendPacket(inFmtCtx, pkt, outFmtCtx);
            // sendPacket(inFmtCtx, pkt, [&](std::unique_ptr<AVFrame> frame) {
            // passFrame(frame);
        }); // send the packet to the decoder
    }
    */
};

int sendPacket(FormatContext& inFmtCtx, FormatContext& outFmtCtx, const AVPacket* pkt){ //, std::function<void(Frame&)>pFrame
    int err = AVERROR(1);

    auto it = inFmtCtx.open_streams.find(pkt->stream_index); // it returns an iterator
    if(it != inFmtCtx.open_streams.end()){
        avcodec_send_packet(it->second.get(), pkt); //avcodec_send_packet(cCtx->second.get(), pkt);
        for(;;){
            //Frame frame = Frame();
            Frame frame = Frame(av_frame_alloc(), [](AVFrame* frame){
                av_frame_free(&frame);
            });
            err = avcodec_receive_frame(it->second.get(), frame.get());
            if(err<0){
                break;
            }
            passFrame(frame, inFmtCtx, outFmtCtx, inFmtCtx.open_streams.find(0)->second.get()->codec_type);
        }
    }
    return err == AVERROR(EAGAIN) ? 0 : err;
};

int sendPacket(FormatContext& inFmtCtx, FormatContext& outFmtCtx, Packet&pkt){
  return sendPacket(inFmtCtx, outFmtCtx, pkt.get());
};

void passFrame(Frame& frame, FormatContext& inCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType){
    Frame convFrame =  Frame(av_frame_alloc(), [](AVFrame* frame) {
        av_frame_free(&frame);
    });

    if(mediaType == AVMEDIA_TYPE_VIDEO) {
        sws_ctx = sws_getContext(
                frame->width, frame->height, (AVPixelFormat) frame->format,
                outFmtCtx.open_streams.find(0)->second.get()->width, outFmtCtx.open_streams.find(0)->second.get()->height, outFmtCtx.open_streams.find(0)->second.get()->pix_fmt,
                SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);


        uint8_t *buffer = (uint8_t *) av_malloc(
                av_image_get_buffer_size(outFmtCtx.open_streams.find(0)->second.get()->pix_fmt, outFmtCtx.open_streams.find(0)->second.get()->width,
                                         outFmtCtx.open_streams.find(0)->second.get()->height,1));
        if (buffer == NULL) {
            std::cerr << "Unable to allocate memory for the buffer" << std::endl;
        }
        if((av_image_fill_arrays(convFrame->data, convFrame->linesize, buffer, outFmtCtx.open_streams.find(0)->second.get()->pix_fmt,
                                 outFmtCtx.open_streams.find(0)->second.get()->width, outFmtCtx.open_streams.find(0)->second.get()->height, 1)) <0){
            std::cerr << "An error occured while filling the image array" << std::endl;
        };

        /*
        convFrame->width = outFmtCtx.open_streams.find(0)->second.get().width;
        convFrame->height = outFmtCtx.open_streams.find(0)->second.get().height;
        convFrame->format = outFmtCtx.open_streams.find(0)->second.get().pix_fmt;

        av_frame_copy_props(convFrame.get(), frame.get());
         */

        sws_scale(sws_ctx, (uint8_t const *const *)frame->data, frame->linesize, 0,
                  frame->height, convFrame->data, convFrame->linesize);

        encode(outFmtCtx, convFrame, mediaType);
    }

    if(mediaType == AVMEDIA_TYPE_AUDIO) {
        swr_ctx = swr_alloc_set_opts(nullptr,
                                     av_get_default_channel_layout(inCtx.open_streams.find(0)->second.get()->channels),
                                     requireAudioFmt,  // aac encoder only receive this format
                                     inCtx.open_streams.find(0)->second.get()->sample_rate,
                                     av_get_default_channel_layout(inCtx.open_streams.find(0)->second.get()->channels),
                                     (AVSampleFormat)inCtx->streams[0]->codecpar->format,
                                     inCtx->streams[0]->codecpar->sample_rate,
                                     0, nullptr);


        uint8_t **cSamples = nullptr;
        int res = av_samples_alloc_array_and_samples(&cSamples, NULL, outFmtCtx.open_streams.find(1)->second.get()->channels, frame->nb_samples, requireAudioFmt, 0);
        if (res < 0) {
            std::cerr << "Fail to alloc samples by av_samples_alloc_array_and_samples." << std::endl;
        }
        res = swr_convert(swr_ctx, cSamples, frame->nb_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
        if (res < 0) {
            std::cerr << "Failed swr_convert." << std::endl;
        }
        if (av_audio_fifo_space(audioFifo) < frame->nb_samples) {
            std::cerr << "Audio buffer is too small." << std::endl;
        }

        res = av_audio_fifo_write(audioFifo, (void**)cSamples, frame->nb_samples);
        if (res < 0) {
            std::cerr << "Fail to write fifo" << std::endl;
        }

        av_freep(&cSamples[0]);

        while (av_audio_fifo_size(audioFifo) >= outFmtCtx.open_streams.find(1)->second.get()->frame_size) {
            convFrame->nb_samples = outFmtCtx.open_streams.find(1)->second.get()->frame_size;
            convFrame->channels = inCtx.open_streams.find(0)->second.get()->channels;
            convFrame->channel_layout = av_get_default_channel_layout(inCtx.open_streams.find(0)->second.get()->channels);
            convFrame->format = requireAudioFmt;
            convFrame->sample_rate = outFmtCtx.open_streams.find(1)->second.get()->sample_rate;

            res = av_frame_get_buffer(convFrame.get(), 0);
            res = av_audio_fifo_read(audioFifo, (void**)convFrame->data, outFmtCtx.open_streams.find(1)->second.get()->frame_size);

            // convFrame->pts = apts++;
            encode(outFmtCtx, convFrame, mediaType);

        }

    }
};

void encode(FormatContext& outFmtCtx, Frame& frame, const AVMediaType& mediaType){
    int res;
    Packet pkt = Packet(outFmtCtx.get());

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        res = avcodec_send_frame(outFmtCtx.open_streams.find(0)->second.get(), frame.get());  //outFmtCtx.open_streams.find(0)->second.get().get()
        if(res < 0){
            std::cerr << "Error sending the frame for encoding, media type " << mediaType << std::endl;
        }
        while (res >= 0){
            res = avcodec_receive_packet(outFmtCtx.open_streams.find(0)->second.get(), pkt.get()); // outFmtCtx.open_streams.find(0)->second.get().get()
            pkt->stream_index = 0;
            writeFrame(outFmtCtx, pkt, mediaType);
        }
    }else if(mediaType == AVMEDIA_TYPE_AUDIO){
        res = avcodec_send_frame(outFmtCtx.open_streams.find(1)->second.get(), frame.get());  //outFmtCtx.open_streams.find(0)->second.get().get()
        if(res < 0){
            std::cerr << "Error sending the frame for encoding, media type " << mediaType << std::endl;
        }
        while (res >= 0){
            res = avcodec_receive_packet(outFmtCtx.open_streams.find(1)->second.get(), pkt.get()); // outFmtCtx.open_streams.find(0)->second.get().get()
            pkt->stream_index = 0;
            writeFrame(outFmtCtx, pkt, mediaType);
        }
    }
};

void generateOutStreams(FormatContext& outFmtCtx, const AVCodecParameters* par, const AVMediaType& mediaType){
    AVStream* out_stream = avformat_new_stream(outFmtCtx.get(), nullptr);
    avcodec_parameters_copy(out_stream->codecpar, par);
    if(mediaType == AVMEDIA_TYPE_VIDEO){
        out_stream->index = 0;
    } else if(mediaType == AVMEDIA_TYPE_AUDIO){
        out_stream->index = 1;
    }
};

int writeFrame(FormatContext& fmtCtx, const Packet& pkt, AVMediaType mediaType){
    int err = 0;
    // this is a flush packet, ignore and return.
    if (!pkt->data || !pkt->size) {
        return AVERROR_EOF;
    }
    AVPacket dup;
    av_packet_ref(&dup, pkt.get());

    dup.stream_index = (mediaType==AVMEDIA_TYPE_VIDEO ? 0 : 1);
    auto& track = fmtCtx->streams[(mediaType==AVMEDIA_TYPE_VIDEO ? 0 : 1)];

    //av_packet_rescale_ts(&dup, FLICKS_TIMESCALE_Q, track->time_base);
    err = ::av_interleaved_write_frame(fmtCtx.get(), &dup);
    av_packet_unref(&dup);
    return err;
};
