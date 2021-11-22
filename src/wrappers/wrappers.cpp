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

void decode(FormatContext& fmtCtx){
    for(auto& pkt : fmtCtx) {
        //send the packet to the decoder
        avcodec_send_packet(fmtCtx, pkt);
    }
};

int sendPacket(FormatContext& fmtCtx, const AVPacket* pkt){
    int err = AVERROR(1);
    auto cCtx = fmtCtx.open_streams.find(pkt->stream_index);
    if(cCtx != fmtCtx.open_streams.end()){
        avcodec_send_packet(cCtx->second.get(), pkt);
        for(;;){
            //Frame frame = Frame();
            Frame frame = Frame(av_frame_alloc(), [](AVFrame* frame){
                av_frame_free(&frame);
            });
            fmtCtx.open_streams
            err = avcodec_receive_frame((cCtx->second.get(), frame.get()));
            if(err<0){
                break;
            }
        }
    }
    return err == AVERROR(EAGAIN) ? 0 : err;
};


