extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
}

#include <stdio.h>
#include <iostream>

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

int main(int argc, char **argv) {
    avformat_network_init();
    avdevice_register_all();

    AVFormatContext *pFormatCtx; //it contains the informations about the format

    pFormatCtx = avformat_alloc_context(); //allocate the memory to the AVFormatContext component
    if (!pFormatCtx) {
        std::cout << "Couldn't create AVFormatContext" << std::endl;
        exit(-1);
    }

    /*
    *   Linux - we'll use "x11grub"
    */
    AVInputFormat *ift = av_find_input_format("x11grab");

    /*
   * avformat_open_input(AVFormatContext, filename, AVInputFormat, AVDictionary)
   * this function is used to open the file and read its header
   * 
   * The last three arguments are used to specify the file format, 
   * buffer size, and format options, but by setting this to nullptr or 0, libavformat will auto-detect these. 
   */
    if (avformat_open_input(&pFormatCtx, ":0.0", ift, nullptr) != 0) {
        std::cout << "Couldn't open the video file" << std::endl;
        exit(-1);
    };

    // Now we retrieve the stream informations. It populates pFormatCtx->streams with the proper infos
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        //the function only looks at the header, so we must check out the stream informations
        std::cout << "Couldn't find stream informations" << std::endl;
        exit(-1);
    }

    // Now pFormatCtx->streams is just an array of pointers, of size pFormatCtx->nb_streams

    // Walk through the streams until we find a video stream
    int videoIndex = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
            break;
        }
    if (videoIndex == -1) {
        std::cout << "Didn't find a video stream." << std::endl;
        exit(-1);
    }

    AVCodecContext *pCodecCtx = nullptr; //it will contain the stream's information about the codec
    AVCodec *pCodec = nullptr;

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        std::cout << "Out of memory error" << std::endl;
        avformat_close_input(&pFormatCtx);
        exit(-1);
    }

    // Get a pointer to the codec context for the video stream
    int result = avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoIndex]->codecpar);
    if (result < 0) {
        //failed to set parameters
        avformat_close_input(&pFormatCtx);
        avcodec_free_context(&pCodecCtx);
    }

    //Now we need to find the actual codec and open it
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == nullptr) {
        std::cout << "Codec not supported." << std::endl;
        exit(-1);
    }

    /* We must not use the AVCodecContext from the video stream directly
    * So we copy the context to a new location, after allocating memory for it.
    *
    * avcodec_alloc_context3()
    * if the codec param is non-nullptr, allocate private data and initialize defaults for the given codec. 
    * It is illegal to then call avcodec_open2() with a different codec. If nullptr, then the codec-specific 
    * defaults won't be initialized, which may result in suboptimal default settings
    */
    /*
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context())
    */

    // open codec. options -> nullptr
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        std::cout << "Could not open the codec." << std::endl;
        pCodecCtx = nullptr;
        exit(-1);
    }

    /* STORING THE DATA */
    AVFrame *pFrame = nullptr;    // the place to actually store the frame
    pFrame = av_frame_alloc(); // allocate video frame
    if (!pFrame) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        exit(-1);
    }
    /*
     * This AVFrame represents the converted frame for the output
     */
    AVFrame *pFrameConv = nullptr;
    pFrameConv = av_frame_alloc(); // allocate video frame
    if (!pFrameConv) {
        std::cout << "Couldn't allocate AVFrame" << std::endl;
        exit(-1);
    }

    uint8_t *buffer = nullptr;
    int nbytes;
    // Determine required buffer size and allocate buffer
    nbytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                      pCodecCtx->height, 32);

    buffer = (uint8_t *) av_malloc(nbytes * sizeof(uint8_t));


    av_image_fill_arrays(pFrameConv->data, pFrameConv->linesize, buffer, pCodecCtx->pix_fmt, pCodecCtx->width,
                         pCodecCtx->height, 1);


    //READING THE DATA
    struct SwsContext *sws_ctx = nullptr;
    int frameFinished;
    AVPacket packet;
// initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width,
                             pCodecCtx->height,
                             pCodecCtx->pix_fmt,
                             pCodecCtx->width,
                             pCodecCtx->height,
                             AV_PIX_FMT_RGB24,
                             SWS_BILINEAR,
                             nullptr,
                             nullptr,
                             nullptr
    );

    int i = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoIndex) {
            // Decode video frame
            //avcodec_send_packet(pCodecCtx, &packet);
            int res = avcodec_send_packet(pCodecCtx, &packet);

            if (res < 0) {
                fprintf(stderr, "Error while sending a packet to the decoder: %d", res);
                return res;
            }

            while (res >= 0) {
                // Return decoded output data (into a frame) from a decoder
                res = avcodec_receive_frame(pCodecCtx, pFrame);

                if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                    break;
                } else if (res < 0) {
                    fprintf(stderr, "Error while receiving a frame from the decoder: %d", res);
                    //return res;
                    break;
                }
                //frameFinished = avcodec_receive_frame(pCodecCtx, pFrame);
                //avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
                printf(
                        "Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d]\n",
                        pCodecCtx->frame_number,
                        av_get_picture_type_char(pFrame->pict_type),
                        pFrame->pkt_size,
                        pFrame->format,
                        pFrame->pts,
                        pFrame->key_frame,
                        pFrame->coded_picture_number
                );
                // Did we get a video frame?
                if (res==0) {
                    // Convert the image from its native format to RGB
                    sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height,
                              pFrameConv->data, pFrameConv->linesize);

                    // Save the frame to disk
                    SaveFrame(pFrameConv, pCodecCtx->width,
                              pCodecCtx->height, i+1);
                }
            }
            if (++i >= 24) {
                break;
            }
        }
    }
    av_packet_unref(&packet);

    // Free the RGB image
    av_free(buffer);
    av_free(pFrameConv);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

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