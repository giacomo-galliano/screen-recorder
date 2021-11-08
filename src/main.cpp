#include <stdio.h>
#include <iostream>
#include "ScreenRecorder.h"

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

int main(int argc, char **argv) {
    avformat_network_init();
    avdevice_register_all();

    AVFormatContext* inFormatCtx = nullptr;
    AVFormatContext* outFormatCtx = nullptr;

    inFormatCtx = avformat_alloc_context(); //allocate the memory to the AVFormatContext component
    if (!inFormatCtx) {
        std::cout << "Couldn't create input AVFormatContext" << std::endl;
        exit(-1);
    }
    AVInputFormat* ift = av_find_input_format("x11grab");

    char* filename_out = "../media/output.mp4";
    AVOutputFormat *oft = av_guess_format(nullptr, filename_out, nullptr);
    if(!oft){
        std::cout << "Can't create output format" << std::endl;
        return -1;
    }

    avformat_alloc_output_context2(&outFormatCtx, oft, NULL, filename_out);
    if (!outFormatCtx) {
        std::cout << "Couldn't create output AVFormatContext" << std::endl;
        exit(-1);
    }
    AVStream* outStream = avformat_new_stream(outFormatCtx, NULL);

    inFormatCtx->probesize = 42000000;

    if (avformat_open_input(&inFormatCtx, ":0.0", ift, nullptr) != 0) {
        std::cout << "Couldn't open the video file" << std::endl;
        exit(-1);
    };

    // Now we retrieve the stream informations. It populates inFormatCtx->streams with the proper infos
    if (avformat_find_stream_info(inFormatCtx, nullptr) < 0) {
        std::cout << "Couldn't find stream informations" << std::endl;
        exit(-1);
    }

    AVDictionary *options = nullptr;
    int val = av_dict_set(&options, "framerate", "48", 0);
    if(val != 0){
        std::cout << "Error setting dictionary value" << std::endl;
    }

    val = av_dict_set(&options, "preset", "medium", 0);
    if(val != 0){
        std::cout << "Error setting dictionary value" << std::endl;
    }
    // Now inFormatCtx->streams is just an array of pointers, of size inFormatCtx->nb_streams


    int videoIndex = -1;
    for (int i = 0; i < inFormatCtx->nb_streams; i++)
        if (inFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    if (videoIndex == -1) {
        std::cout << "Didn't find a video stream." << std::endl;
        exit(-1);
    }

/*
    int videoIndex = av_find_best_stream(inFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int audioIndex = av_find_best_stream(inFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if(videoIndex<0){
        std::cout << "Didn't find a video stream." << std::endl;
        exit(-1);
    }
*/

    /*
    if(audioIndex<0){
        std::cout << "Didn't find an audio stream." << std::endl;
        //exit(-1);
    }
     */


    AVStream* videoStream = inFormatCtx->streams[videoIndex];
    //AVStream* audioStream = inFormatCtx->streams[audioIndex];

    /*
     * ######## DECODER #########
     */
    AVCodecContext *decoderCtx = nullptr; //this is a structure
    AVCodec *decoder = nullptr; //it describes the decoder

    decoderCtx = avcodec_alloc_context3(decoder);
    if (!decoderCtx) {
        std::cout << "Error allocating decoder context" << std::endl;
        avformat_close_input(&inFormatCtx);
        exit(-1);
    }

    // Get a pointer to the codec context for the video stream
    int res = avcodec_parameters_to_context(decoderCtx, videoStream->codecpar);
    if (res < 0) {
        //failed to set parameters
        avformat_close_input(&inFormatCtx);
        avcodec_free_context(&decoderCtx);
    }

    //Now we need to find the actual codec and open it
    decoder = avcodec_find_decoder(decoderCtx->codec_id);
    if (decoder == nullptr) {
        std::cout << "Codec not supported." << std::endl;
        exit(-1);
    }

    if (avcodec_open2(decoderCtx, decoder, nullptr) < 0) {
        std::cout << "Could not open the decoder." << std::endl;
        decoderCtx = nullptr;
        exit(-1);
    }


    /*
     * ########## ENCODER #########
     */

    AVCodecContext* encoderCtx = nullptr;
    AVCodec* encoder = nullptr;

    encoderCtx = avcodec_alloc_context3(encoder);
    if (!encoderCtx) {
        std::cout << "Error allocating encoder context" << std::endl;
        exit(-1);
    }

    //avcodec_parameters_copy(outStream->codecpar, videoStream->codecpar);

    encoderCtx = outStream->codec;
    encoderCtx->codec_id = AV_CODEC_ID_MPEG4;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
    encoderCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    encoderCtx->pix_fmt  = AV_PIX_FMT_YUV420P;
    encoderCtx->bit_rate = 2500000;
    encoderCtx->width = 1920;
    encoderCtx->height = 1080;
    encoderCtx->gop_size = 3;
    encoderCtx->max_b_frames = 2;
    encoderCtx->time_base.num = 1;
    encoderCtx->time_base.den = 150; // 30->15fps
    
    encoder = avcodec_find_encoder((AV_CODEC_ID_MPEG4));
    if(!encoder){
        std::cout << "An error occurred trying to find the encoder codec" << std::endl;
    }

    /* Some container formats (like MP4) require global headers to be present
   Mark the encoder so that it behaves accordingly. */
    if ( outFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        outFormatCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(encoderCtx, encoder, nullptr) < 0) {
        std::cout << "Could not open the encoder." << std::endl;
        decoderCtx = nullptr;
        exit(-1);
    }

    if (!(oft->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&outFormatCtx->pb, filename_out, AVIO_FLAG_WRITE);
        //int ret = avio_open2(&outFormatCtx->pb, filename_out, AVIO_FLAG_WRITE, NULL, NULL);
        if (ret < 0) {
            std::cout << "Could not open output file " << filename_out << std::endl;
            return -1;
        }
    }
    int ret = avformat_write_header(outFormatCtx, &options);
    if (ret < 0) {
        std::cout << "Failed to write header" << std::endl;
        return -1;
    }

    av_dump_format(outFormatCtx, 0, filename_out, 1);


    /*
     * ############ DECODING ###########
     */

    AVPacket packet;
    /*
     AVPacket* packet = (AVPacket*)av_packet_alloc();
    if(packet == NULL){
        std::cout << "error allocating the packet" << std::endl;
    }
     */
    AVFrame* frame = av_frame_alloc(); // allocate video frame
    if (!frame) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        exit(-1);
    }
    AVFrame *pFrameConv = av_frame_alloc(); // allocate video frame
    if (!pFrameConv) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        exit(-1);
    }

    struct SwsContext *sws_ctx = nullptr;
    int frameFinished;
    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(decoderCtx->width,
                             decoderCtx->height,
                             decoderCtx->pix_fmt,
                             encoderCtx->width,
                             encoderCtx->height,
                             encoderCtx->pix_fmt,
                             SWS_BILINEAR,
                             nullptr,
                             nullptr,
                             nullptr
    );


    // Determine required buffer size and allocate buffer
    int n_bytes = av_image_get_buffer_size(encoderCtx->pix_fmt, encoderCtx->width,
                                      encoderCtx->height, 32);
    uint8_t *buffer = nullptr;
    buffer = (uint8_t *) av_malloc(n_bytes * sizeof(uint8_t));
    if(buffer==NULL){
        std::cout << "unable to allocate memory for the buffer" << std::endl;
        exit(-1);
    }
    if((av_image_fill_arrays(pFrameConv->data, pFrameConv->linesize, buffer, encoderCtx->pix_fmt, encoderCtx->width,
                         encoderCtx->height, 1)) <0){
        std::cout << "An error occured while filling the image array" << std::endl;
        exit(-1);
    };

    int index = 0;
    int nframe = 200;
    //loop as long we have a frame to read
    while (av_read_frame(inFormatCtx, &packet) == 0) {
        if(index++ == nframe){
            break;
        }
        // JUST CHECKING VIDEO! NEED TO MODIFY FOR AUDIO
        if(packet.stream_index != videoStream->index) continue;
        res = avcodec_send_packet(decoderCtx, &packet);
        if(res<0){
            std::cout << "An error happened during the decoding phase" <<std::endl;
            exit(-1);
        }
        while((res = avcodec_receive_frame(decoderCtx, frame)) == 0){
            if (res == AVERROR(EAGAIN) || res == AVERROR_EOF)
                return -1;
            if (res< 0) {
                std::cout << "Error during encoding" << std::endl;
                exit(1);
            }

            pFrameConv->width=encoderCtx->width;
            pFrameConv->height=encoderCtx->height;
            pFrameConv->format = encoderCtx->pix_fmt;

            sws_scale(sws_ctx, (uint8_t const *const *) frame->data,
                      frame->linesize, 0, decoderCtx->height,
                      pFrameConv->data, pFrameConv->linesize);

            res = avcodec_send_frame(encoderCtx, pFrameConv);
            if (res < 0) {
                fprintf(stderr, "Error sending a frame for encoding\n");
                exit(1);
            }
        }
        AVPacket outPacket;

        if ((res = avcodec_receive_packet(encoderCtx, &outPacket))==0) {
            if (res == AVERROR(EAGAIN) || res== AVERROR_EOF){
                return (res==AVERROR(EAGAIN)) ? 0:1;
            }
            else if (res< 0) {
                std::cout << "Error during encoding" << std::endl;
                exit(1);
            }

            if (encoderCtx->coded_frame->pts != AV_NOPTS_VALUE)
                outPacket.pts= av_rescale_q(encoderCtx->coded_frame->pts, encoderCtx->time_base, videoStream->time_base);
            if(outPacket.dts != AV_NOPTS_VALUE)
                outPacket.dts = av_rescale_q(outPacket.dts, encoderCtx->time_base, encoderCtx->time_base);
            if(encoderCtx->coded_frame->key_frame)
                outPacket.flags |= AV_PKT_FLAG_KEY;

            res= av_interleaved_write_frame(outFormatCtx, &outPacket);

            if (res< 0) {
                fprintf(stderr, "Error while writing video frame: %d\n", res);
                exit(1);
            }
            av_packet_unref(&outPacket);
        }
    }

    av_write_trailer(outFormatCtx);
    if (!(oft->flags & AVFMT_NOFILE)) {
        int err = avio_close(outFormatCtx->pb);
        if (err < 0) {
            std::cout << "Failed to close file" << std::endl;
        }
    }

    return 0;
}
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==nullptr)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

    // Close file
    fclose(pFile);
}