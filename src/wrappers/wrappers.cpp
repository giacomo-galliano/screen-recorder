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

int prepareDecoder(FormatContext& fmtCtx, AVMediaType mediaType){
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

int prepareEncoder(FormatContext* fmtCtx, AVMediaType mediaType){
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

    }

    if(avcodec_open2(cCtx.get(), codec, nullptr)<0){
        std::cerr << "Failed to initialize the decoder context for media type " << mediaType << std::endl;
        return -1;
    }

    if(mediaType == AVMEDIA_TYPE_VIDEO){
        fmtCtx->open_streams.emplace(0, std::move(cCtx));
    }else if(mediaType == AVMEDIA_TYPE_AUDIO) {
        fmtCtx->open_streams.emplace(1, std::move(cCtx));
    }
};

void decode(FormatContext& fmtCtx){
    for(auto& pkt : fmtCtx) {
        sendPacket(fmtCtx, pkt, [&](std::unique_ptr<AVFrame> frame) {
            passFrame(frame);
        }); // send the packet to the decoder
    }
};

int sendPacket(FormatContext& fmtCtx, const AVPacket* pkt, std::function<void(Frame&)>pFrame){
    int err = AVERROR(1);

    auto cCtx = fmtCtx.open_streams.find(pkt->stream_index); // it returns an iterator
    if(cCtx != fmtCtx.open_streams.end()){
        avcodec_send_packet(&cCtx->second, pkt); //avcodec_send_packet(cCtx->second.get(), pkt);
        for(;;){
            //Frame frame = Frame();
            Frame frame = Frame(av_frame_alloc(), [](AVFrame* frame){
                av_frame_free(&frame);
            });
            err = avcodec_receive_frame(&cCtx->second, frame.get());
            if(err<0){
                break;
            }
            pFrame(frame);
        }
    }
    return err == AVERROR(EAGAIN) ? 0 : err;
};

int sendPacket(FormatContext& fmtCtx, Packet&pkt, std::function<void(Frame&)>pFrame){
  return sendPacket(fmtCtx, pkt.get(), pFrame);
};

void passFrame(std::unique_ptr<AVFrame>& frame){
    sws_ctx = sws_getContext(
            frame->width, frame->height, (AVPixelFormat)frame->format,
            frame->width, frame->height, AV_PIX_FMT_YUV420P,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    uint8_t *buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(->pix_fmt, vEncoderCCtx->width,vEncoderCCtx->height, 1));
    if(buffer==NULL){
        std::cout << "unable to allocate memory for the buffer" << std::endl;
        return -1;
    }
};
