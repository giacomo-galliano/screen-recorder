#include "ScreenRecorder.h"

ScreenRecorder::ScreenRecorder() : recording(false), pause(false), finished(false){
    init();
    vPTS = 0;
    aPTS = 0;
}

void ScreenRecorder::open_(){
    switch(rec_type){
        case Command::vofs:
            v_inFmtCtx = openInput(AVMEDIA_TYPE_VIDEO);
            break;
        case Command::avfs:
            v_inFmtCtx = openInput(AVMEDIA_TYPE_VIDEO);
            a_inFmtCtx = openInput(AVMEDIA_TYPE_AUDIO);
            break;
        case Command::vosp:
            //Point p;
            //FormatSize fs;
            //setScreenPortion(p, fs);
            //v_inFmtCtx = openInput();
            break;
        case Command::avsp:
            //v_inFmtCtx = openInput();
            //a_inFmtCtx = openInput();
            break;
        case Command::stop:
            break;
        default:
            std::cout << "Command not recognized" << std::endl;
    }
    if(rec_type != Command::stop){
        std::string out_filename;
        getFilenameOut(out_filename);
        outFmtCtx = openOutput(out_filename);
    }

}

void ScreenRecorder::start_(){
    switch(rec_type){
        case Command::vofs:
            prepareDecoder(v_inFmtCtx, AVMEDIA_TYPE_VIDEO);
            prepareEncoder(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
            writeHeader(outFmtCtx);
            videoThread = new std::thread([this](){
                this->recording.store(true);
                std::cout << "Recording.." << std::endl;
                this->decode(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
            });
            break;
        case Command::avfs:
            prepareDecoder(v_inFmtCtx, AVMEDIA_TYPE_VIDEO);
            prepareDecoder(a_inFmtCtx, AVMEDIA_TYPE_AUDIO);
            prepareEncoder(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
            prepareEncoder(a_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_AUDIO);
            writeHeader(outFmtCtx);
            videoThread = new std::thread([this](){
                this->recording.store(true);
                this->writing.store(false);
                this->decode(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
            });
//            std::this_thread::sleep_for(std::chrono::seconds(2));
            audioThread = new std::thread([this](){
                this->decode(a_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_AUDIO);
            });
            std::cout << "Recording.." << std::endl;
            break;
        case Command::vosp:
            //Point p;
            //FormatSize fs;
            //setScreenPortion(p, fs);
            //v_inFmtCtx = openInput();
            break;
        case Command::avsp:
            //v_inFmtCtx = openInput();
            //a_inFmtCtx = openInput();
            break;
        default:
            std::cout << "Command not recognized" << std::endl;
    }

        PSRMenu();

}

void ScreenRecorder::pause_(){
    this->pause.store(true);
    std::cout << "Recording paused.." << std::endl;
}

void ScreenRecorder::restart_(){
    this->pause.store(false);
    std::cout << "Recording.." << std::endl;
}

void ScreenRecorder::stop_(){
    this->recording.store(false);
    this->finished.store(true);
    switch(rec_type){
        case Command::vofs:
            videoThread->join();
            break;
        case Command::avfs:
            videoThread->join();
            audioThread->join();
            break;
        case Command::vosp:
            videoThread->join();
            break;
        case Command::avsp:
            videoThread->join();
            audioThread->join();
            break;
        default:
            std::cout << "Command not recognized" << std::endl;
    }

    int err = writeTrailer(outFmtCtx);
    if(err < 0)throw std::runtime_error("A problem occurred writing the file trailer.");
    std::cout << "Recording stopped." << std::endl;

}


void ScreenRecorder::init(){
    avformat_network_init();
    avdevice_register_all();
}

int ScreenRecorder::readFrame(AVFormatContext* fmtCtx, AVPacket* pkt){
    if(!fmtCtx || !pkt){
        return AVERROR(1);
    }
    int err = av_read_frame(fmtCtx, pkt);
    if(err >= 0){
        //auto& stream = fmtCtx->streams[pkt->stream_index];
        //av_packet_rescale_ts(pkt, stream->time_base, FLICKS_TIMESCALE_Q);
    }else{
//        av_init_packet(pkt);
//        pkt->size = 0;
//        pkt->data = nullptr;
    }
    return err;
}

int ScreenRecorder::writeHeader(FormatContext& fmtCtx){

    if (avformat_write_header(fmtCtx.get(), nullptr) < 0) {
        std::cout << "Failed to write header" << std::endl;
        return -2;
    }
    return 0;
};

int ScreenRecorder::writeTrailer(FormatContext& fmtCtx){
    av_write_trailer(fmtCtx.get());
    if (!(fmtCtx->flags & AVFMT_NOFILE)) {
        int err = avio_close(fmtCtx->pb);
        if (err < 0) {
            std::cout << "Failed to close file" << std::endl;
            return -1;
        }
    }
    return 0;
};


int ScreenRecorder::prepareDecoder(FormatContext& fmtCtx, AVMediaType mediaType){
    AVCodec* codec = nullptr;
    int index = -1;

    for(int i=0; i < fmtCtx->nb_streams; i++){
        if (fmtCtx->streams[i]->codecpar->codec_type == mediaType){
            index = i;
        }
    }
    if (index == -1) {
        std::cerr << "Media does not contain any usable stream of type " << mediaType << std::endl;
        return -1;
    }

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        in_v_index = index;
    }else if(mediaType == AVMEDIA_TYPE_AUDIO){
        in_a_index = index;
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

int ScreenRecorder::prepareEncoder(FormatContext& inFmtCtx, FormatContext& outFmtCtx, AVMediaType mediaType){
    AVCodec* codec = nullptr;
    if(mediaType == AVMEDIA_TYPE_VIDEO){
        codec = avcodec_find_encoder(AV_CODEC_ID_H264); //TODO: scegliere tra H264 e MPEG4
    }else if(mediaType == AVMEDIA_TYPE_AUDIO){
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    }
    if(!codec){
        std::cerr << "Failed to find the ecnoder codec for media type " << mediaType << std::endl;
        return -1;
    }
    //TODO: non sarebbe meglio mettere find encoder nell'inizializer di CodecContex?

    CodecContext cCtx = CodecContext (avcodec_alloc_context3(codec), [](AVCodecContext *ctx){
        avcodec_free_context(&ctx);
    });

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        cCtx->width = inFmtCtx.open_streams.find(in_v_index)->second.get()->width;
        cCtx->height = inFmtCtx.open_streams.find(in_v_index)->second.get()->height;
        cCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        cCtx->time_base = (AVRational){1,6000};
        cCtx->framerate = (AVRational){60,1};//av_inv_q(cCtx->time_base);
//        cCtx->bit_rate = 4000000;
//        cCtx->gop_size = 10;
//        cCtx->max_b_frames = 0;
//        cCtx->profile = FF_PROFILE_H264_MAIN;
        if ( outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
            cCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        cCtx->channels = inFmtCtx.open_streams.find(in_a_index)->second.get()->channels;
        cCtx->channel_layout = av_get_default_channel_layout(inFmtCtx.open_streams.find(in_a_index)->second.get()->channels);
        cCtx->sample_rate = inFmtCtx.open_streams.find(in_a_index)->second.get()->sample_rate;
        cCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
//        cCtx->frame_size = 2048;
        cCtx->bit_rate = 128000;
        cCtx->time_base.num = 1;
        cCtx->time_base.den = cCtx->sample_rate;
//        cCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    generateOutStreams(outFmtCtx, cCtx, mediaType);

    if(avcodec_open2(cCtx.get(), codec, nullptr)<0){
        std::cerr << "Failed to initialize the decoder context for media type " << mediaType << std::endl;
        return -1;
    }

    avcodec_parameters_from_context(outFmtCtx->streams[mediaType == AVMEDIA_TYPE_VIDEO ? OUT_VIDEO_INDEX : OUT_AUDIO_INDEX]->codecpar, cCtx.get());

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        outFmtCtx.open_streams.emplace(OUT_VIDEO_INDEX, std::move(cCtx));
    }else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        outFmtCtx.open_streams.emplace(OUT_AUDIO_INDEX, std::move(cCtx));
    }

    return 0;
}

void ScreenRecorder::generateOutStreams(FormatContext& outFmtCtx, const CodecContext& cCtx, const AVMediaType& mediaType){
    AVStream* out_stream = avformat_new_stream(outFmtCtx.get(), nullptr);
//    avcodec_parameters_from_context(out_stream->codecpar, cCtx.get()); --> non va bene qui, messa dopo la open
    if(mediaType == AVMEDIA_TYPE_VIDEO){
        out_stream->index = 0;
    } else if(mediaType == AVMEDIA_TYPE_AUDIO){
        out_stream->index = 1;
    }
}

void ScreenRecorder::decode(FormatContext& inFmtCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType){

    Packet pkt = Packet(inFmtCtx.get());

//    std::unique_lock<std::mutex> ul(m);

    while(true){
//        cv.wait(ul, [this] (){return !pause;});
        if(finished.load()){
            break;
        }
//        ul.unlock();

        if(readFrame(inFmtCtx.get(), pkt.get()) >= 0){

//            std::cout << "READFR: " << " p->" << pkt->pts << std::endl;
//            std::cout << "READFR: " << " p->" << pkt->pts << std::endl;
            if(mediaType == AVMEDIA_TYPE_VIDEO) {
                if(vPTS == 0){
                    vPTS = pkt->pts;
                    pkt->pts = pkt->dts = 0;
                }else {
                    pkt->pts = pkt->dts = (pkt->pts - vPTS );
                }
                pkt->pts = av_rescale_q(pkt->pts, inFmtCtx->streams[in_v_index]->time_base,
                                        outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->time_base);
                pkt->dts = av_rescale_q(pkt->dts, inFmtCtx->streams[in_v_index]->time_base,
                                        outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->time_base);
                pkt->duration = av_rescale_q(pkt->duration, inFmtCtx->streams[in_v_index]->time_base,
                                             outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->time_base);
            }else if (mediaType == AVMEDIA_TYPE_AUDIO){
                if(aPTS == 0){
                    aPTS = pkt->pts;
                    pkt->pts = pkt->dts = 0;
                }else {
                    pkt->pts = pkt->dts = (pkt->pts - aPTS );
                }
                pkt->pts = av_rescale_q(pkt->pts, inFmtCtx->streams[in_a_index]->time_base,
                                        outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->time_base);
                pkt->dts = av_rescale_q(pkt->dts, inFmtCtx->streams[in_a_index]->time_base,
                                        outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->time_base);
                pkt->duration = av_rescale_q(pkt->duration, inFmtCtx->streams[in_a_index]->time_base,
                                             outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->time_base);

            }
            sendPacket(inFmtCtx, outFmtCtx, pkt.get());
            //mia modifica
            av_packet_unref(pkt.get());
            //
        }
    }
}

int ScreenRecorder::sendPacket(FormatContext& inFmtCtx, FormatContext& outFmtCtx, AVPacket* pkt){ //, std::function<void(Frame&)>pFrame
    int err = AVERROR(1);
    //spostato da dentro il for
    //Frame frame = Frame();
    Frame frame = Frame(av_frame_alloc(), [](AVFrame* frame){
        av_frame_free(&frame);
    });
    //
//    int64_t time = av_gettime();

    auto it = inFmtCtx.open_streams.find(pkt->stream_index); // it returns an iterator
    if(it != inFmtCtx.open_streams.end()){

        avcodec_send_packet(it->second.get(), pkt); //avcodec_send_packet(cCtx->second.get(), pkt);

        for(;;){
            err = avcodec_receive_frame(it->second.get(), frame.get());
            if(err<0){
                break;
            }

            if (frame->pts == AV_NOPTS_VALUE) {
                std::cout << "NO VALUE PTS" << std::endl;
                break;
            }

//            std::cout << "SENDPK: " << " pictype->" << frame->pict_type << " f->" << frame->pts << " p->" << pkt->pts << std::endl;
            passFrame(frame, inFmtCtx, outFmtCtx, it->second.get()->codec_type);
            //mia modifica
            av_frame_unref(frame.get());
            if (it->second.get()->codec_type == AVMEDIA_TYPE_AUDIO){
                break;
            }
        }
    }
    return err == AVERROR(EAGAIN) ? 0 : err;
};

void ScreenRecorder::passFrame(Frame& frame, FormatContext& inCtx, FormatContext& outFmtCtx, const AVMediaType& mediaType){
    Frame convFrame =  Frame(av_frame_alloc(), [](AVFrame* frame) {
        av_frame_free(&frame);
    });

    if(mediaType == AVMEDIA_TYPE_VIDEO) {
        sws_ctx = sws_getContext(
                frame->width, frame->height, (AVPixelFormat) frame->format,
                outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->width, outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->height,
                outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->pix_fmt,
                SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);


        uint8_t *buffer = (uint8_t *) av_malloc(
                av_image_get_buffer_size(outFmtCtx.open_streams.find(in_v_index)->second.get()->pix_fmt, outFmtCtx.open_streams.find(in_v_index)->second.get()->width,
                                         outFmtCtx.open_streams.find(in_v_index)->second.get()->height,1));
        if (buffer == NULL) {
            std::cerr << "Unable to allocate memory for the buffer" << std::endl;
        }
        if((av_image_fill_arrays(convFrame->data, convFrame->linesize,
                                 buffer, outFmtCtx.open_streams.find(in_v_index)->second.get()->pix_fmt,
                                 outFmtCtx.open_streams.find(in_v_index)->second.get()->width,
                                 outFmtCtx.open_streams.find(in_v_index)->second.get()->height, 1)) <0){
            std::cerr << "An error occured while filling the image array" << std::endl;
        };


        convFrame->width = outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->width;
        convFrame->height = outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->height;
        convFrame->format = outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->pix_fmt;

        av_frame_copy_props(convFrame.get(), frame.get());


        sws_scale(sws_ctx, (uint8_t const *const *)frame->data, frame->linesize, 0,
                  frame->height, convFrame->data, convFrame->linesize);

        encode(outFmtCtx, convFrame, mediaType);
        //mia modifica
        av_frame_unref(convFrame.get());
        av_free(buffer);
        sws_freeContext(sws_ctx);
        //
    }
    else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        swr_ctx = swr_alloc_set_opts(nullptr,
                                     av_get_default_channel_layout(inCtx.open_streams.find(in_a_index)->second.get()->channels),
                                     requireAudioFmt,  // aac encoder only receive this format
                                     inCtx.open_streams.find(in_a_index)->second.get()->sample_rate,
                                     av_get_default_channel_layout(inCtx.open_streams.find(in_a_index)->second.get()->channels),
                                     (AVSampleFormat)inCtx->streams[in_a_index]->codecpar->format,
                                     inCtx->streams[in_a_index]->codecpar->sample_rate,
                                     0, nullptr);
        swr_init(swr_ctx);
        audioFifo = av_audio_fifo_alloc(requireAudioFmt, inCtx.open_streams.find(in_a_index)->second.get()->channels,
                                        inCtx.open_streams.find(in_a_index)->second.get()->sample_rate*2);
        uint8_t **cSamples = nullptr;
        int res = av_samples_alloc_array_and_samples(&cSamples, NULL, outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->channels, frame->nb_samples, requireAudioFmt, 0);
        if (res < 0) {
            std::cerr << "Fail to alloc samples by av_samples_alloc_array_and_samples." << std::endl;
        }
        res = swr_convert(swr_ctx, cSamples, frame->nb_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
        if (res < 0) {
            std::cerr << "Failed swr_convert." << std::endl;
        }
        if (av_audio_fifo_space(audioFifo) < frame->nb_samples) {
            std::cerr << "Audio buffer is too small." << std::endl;
            //TODO:  av_audio_fifo_realloc(audioFifo, av_audio_fifo_size(fifo) + frame->nb_samples)
        }
        res = av_audio_fifo_write(audioFifo, (void**)cSamples, frame->nb_samples);
        if (res < 0) {
            std::cerr << "Fail to write fifo" << std::endl;
        }
        av_freep(&cSamples[0]);

//      int nFrames = frame->pkt_duration / outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->frame_size;
        int i = 0;

        av_frame_copy_props(convFrame.get(), frame.get());
        convFrame->nb_samples = outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->frame_size;
        convFrame->channels = inCtx.open_streams.find(in_a_index)->second.get()->channels;
        convFrame->channel_layout = av_get_default_channel_layout(inCtx.open_streams.find(in_a_index)->second.get()->channels);
        convFrame->format = requireAudioFmt;
        convFrame->sample_rate = outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->sample_rate;
        res = av_frame_get_buffer(convFrame.get(), 0);

        while (av_audio_fifo_size(audioFifo) >= outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->frame_size) {

            convFrame->pts = frame->pts + convFrame->nb_samples * ++i;

            res = av_audio_fifo_read(audioFifo, (void**)convFrame->data, outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->frame_size);

            std::cout << "pts: " << convFrame->pts << " i: " << i << " pkt duration: " << frame->pkt_duration << " samples: " << convFrame->nb_samples << " sample format-> " << convFrame->format << std::endl;
            encode(outFmtCtx, convFrame, mediaType);
        }
            //choose between last part of packet or a single packet whit size < frame_size (scelgo last part --l'altro è come se fosse corrotto)
            if(i > 0) {
            std::cout << "fifo size -> " << av_audio_fifo_size(audioFifo) << std::endl;

            convFrame->pts = frame->pts + convFrame->nb_samples * i + av_audio_fifo_size(audioFifo);

            res = av_audio_fifo_read(audioFifo, (void **) convFrame->data,
                                     outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->frame_size);

            std::cout << "pts: " << convFrame->pts << " i: " << i << " pkt duration: " << frame->pkt_duration
                      << " samples: " << convFrame->nb_samples << " sample formatt-> " << convFrame->format
                      << std::endl;
            if(res > 0)
                encode(outFmtCtx, convFrame, mediaType);
            }
        av_frame_unref(convFrame.get());
        av_audio_fifo_free(audioFifo);
        swr_free(&swr_ctx);
    }
}

void ScreenRecorder::encode(FormatContext& outFmtCtx, Frame& frame, const AVMediaType& mediaType){
    int res;
    Packet pkt = Packet(outFmtCtx.get());

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        //la send frame da l'indicazione sui non monotonic pts
        res = avcodec_send_frame(outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get(), frame.get());  //outFmtCtx.open_streams.find(0)->second.get().get()
        if(res < 0){
            std::cerr << "Error sending the frame for encoding, media type " << mediaType << std::endl;
        }
        while (res >= 0){
            res = avcodec_receive_packet(outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get(), pkt.get()); // outFmtCtx.open_streams.find(0)->second.get().get()
            //mia modifica
            if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                break;
            }
            pkt->stream_index = OUT_VIDEO_INDEX;
//            std::cout << "ENCODE: " << " pictype->" << frame->pict_type << " f->" << frame->pts << " p->" << pkt->pts << std::endl;
            //
            writeFrame(outFmtCtx, pkt, mediaType);
            //mia modifica
            av_packet_unref(pkt.get());
            //
        }
    }else if(mediaType == AVMEDIA_TYPE_AUDIO){

        res = avcodec_send_frame(outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get(), frame.get());  //outFmtCtx.open_streams.find(0)->second.get().get()
        if(res < 0){
            std::cerr << "Error sending the frame for encoding, media type " << mediaType << " frame pts " << frame->pts << std::endl;
        }
//        while (res >= 0){
            res = avcodec_receive_packet(outFmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get(), pkt.get()); // outFmtCtx.open_streams.find(0)->second.get().get()
            if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                return;
            }
            pkt->stream_index = OUT_AUDIO_INDEX;

            writeFrame(outFmtCtx, pkt, mediaType);

            av_packet_unref(pkt.get());
//        }
    }
}

void ScreenRecorder::writeFrame(FormatContext& fmtCtx, Packet& pkt, AVMediaType mediaType){
    // this is a flush packet, ignore and return.
    /*
    if (!pkt->data || !pkt->size) {
        return AVERROR_EOF;
    }
    */

    //mia modifica
    if(mediaType == AVMEDIA_TYPE_VIDEO){
//        AVPacket dup;
//        av_packet_ref(&dup, pkt.get());
//
//        dup.stream_index = (mediaType == AVMEDIA_TYPE_VIDEO ? 0 : 1);
//        auto &track = fmtCtx->streams[(mediaType == AVMEDIA_TYPE_VIDEO ? 0 : 1)];

        pkt->pts = av_rescale_q(pkt->pts, outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->time_base, outFmtCtx->streams[OUT_VIDEO_INDEX]->time_base);
        pkt->dts = av_rescale_q(pkt->dts,outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->time_base, outFmtCtx->streams[OUT_VIDEO_INDEX]->time_base);
        pkt->duration = av_rescale_q(pkt->duration, outFmtCtx.open_streams.find(OUT_VIDEO_INDEX)->second.get()->time_base, outFmtCtx->streams[OUT_VIDEO_INDEX]->time_base);

//        std::cout << "WRITEF: " << " p->" << pkt->pts << std::endl;

    }else if(mediaType == AVMEDIA_TYPE_AUDIO){
        pkt->pts = av_rescale_q(pkt->pts, fmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->time_base, outFmtCtx->streams[OUT_AUDIO_INDEX]->time_base);
        pkt->dts = av_rescale_q(pkt->dts,fmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->time_base, outFmtCtx->streams[OUT_AUDIO_INDEX]->time_base);
        pkt->duration = av_rescale_q(pkt->duration, fmtCtx.open_streams.find(OUT_AUDIO_INDEX)->second.get()->time_base, outFmtCtx->streams[OUT_AUDIO_INDEX]->time_base);

//        std::cout << "AUDIOWRITE: " << " p->" << pkt->pts << " d-> " << pkt->dts << " duration-> " << pkt->duration << std::endl;

//        av_packet_unref(pkt.get());
    }

//        pkt is now blank (av_interleaved_write_frame() takes ownership of
//        * its contents and resets pkt), so that no unreferencing is necessary.
//        av_packet_unref(&dup);
    while(writing.load());
    writing.store(true);
    if(av_interleaved_write_frame(fmtCtx.get(), pkt.get()) != 0){
        //throw
    }
    writing.store(false);

};

void ScreenRecorder::PSRMenu(){
    unsigned short res;
    while(!finished){
        showPSROptions();
        res = getPSRAnswer();
        switch(res){
            case 0:
                stop_();
                break;
            case 1:
                pause_();
                break;
            case 2:
                restart_();
                break;
            default:
                std::cout << "Command not recognized" << std::endl;
        }
    }
};

void ScreenRecorder::showPSROptions(){
    if(pause == false) {
        std::cout << "Digit \"p\" or \"pause\" to pause the recording, \"s\" or \"stop\" to terminate\n"
                  << ">> ";
    }else{
        std::cout << "Digit \"r\" or \"restart\" to resume the recording, \"s\" or \"stop\" to terminate\n"
                  << ">> ";
    }
};

bool ScreenRecorder::validPSRAnswer(std::string &answer, int &res){
    std::transform(answer.begin(), answer.end(), answer.begin(), [](char c){return tolower(c);});

    bool valid_ans = false;
    if(answer == "s" || answer == "stop"){
        valid_ans = true;
        res = 0;
    }else if (answer == "p" || answer == "pause"){
        valid_ans = true;
        res = 1;
    }else if (answer == "r" || answer == "restart"){
        valid_ans = true;
        res = 2;
    }
    return valid_ans;
}

int ScreenRecorder::getPSRAnswer(){
    std::string user_answer;
    int res = -1;

    while(std::cin >> user_answer && (!validPSRAnswer(user_answer, res))){
        std::cout << "Invalid answer " << user_answer << "\nTry again.." << std::endl;
    }
    if(!std::cin){
        //throw std::runtime_error("Failed to read user input");
    }

    return res;
}

void ScreenRecorder::getFilenameOut(std::string& str){
    std::string filename;

    std::cout << "Inserire nome del file output (senza spazi):\n>>  ";
    std::cin >> filename;
    // std::getline(std::cin, filename);
    // std::replace(filename.begin(), filename.end(), ' ', '_');
    str = "../media/" + filename + ".mp4";
}
