extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

#include <stdio.h>
#include <iostream>


int main(int argc, char **argv)
{
    avformat_network_init();
    avdevice_register_all();
    
    AVFormatContext *pFormatCtx; //it contains the informations about the format

    pFormatCtx = avformat_alloc_context(); //allocate the memory to the AVFormatContext component
    if (!pFormatCtx)
    {
        std::cout<<"Couldn't create AVFormatContext"<<std::endl;
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
   * buffer size, and format options, but by setting this to NULL or 0, libavformat will auto-detect these. 
   */
    if (avformat_open_input(&pFormatCtx, ":0.0", ift, NULL) != 0)
    {
        std::cout<<"Couldn't open the video file"<<std::endl;
        exit(-1);
    };

    // Now we retrieve the stream informations. It populates pFormatCtx->streams with the proper infos
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        //the function only looks at the header, so we must check out the stream informations
        std::cout<<"Couldn't find stream informations"<<std::endl;
        exit(-1);
    }

    // Now pFormatCtx->streams is just an array of pointers, of size pFormatCtx->nb_streams

    // Walk through the streams until we find a video stream
    int videoIndex = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
            break;
        }
    if (videoIndex == -1)
    {
        std::cout<<"Didn't find a video stream."<<std::endl;
        exit(-1);
    }

    AVCodecContext *pCodecCtx = NULL; //it will contain the stream's information about the codec
    AVCodec *pCodec = NULL;

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(!pCodecCtx){
        std::cout<<"Out of memory error"<<std::endl;
        avformat_close_input(&pFormatCtx);
        exit(-1);
    }

    // Get a pointer to the codec context for the video stream
    int result = avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoIndex]->codecpar);
    if (result < 0)
    {
        //failed to set parameters
        avformat_close_input(&pFormatCtx);
        avcodec_free_context(&pCodecCtx);
    }

    //Now we need to find the actual codec and open it
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        std::cout<<"Codec not supported."<<std::endl;
        exit(-1);
    }

    /* We must not use the AVCodecContext from the video stream directly
    * So we copy the context to a new location, after allocating memory for it.
    *
    * avcodec_alloc_context3()
    * if the codec param is non-NULL, allocate private data and initialize defaults for the given codec. 
    * It is illegal to then call avcodec_open2() with a different codec. If NULL, then the codec-specific 
    * defaults won't be initialized, which may result in suboptimal default settings
    */
    /*
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context())
    */

    // open codec. options -> NULL
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        std::cout<<"Could not open the codec."<<std::endl;
        pCodecCtx = nullptr;
        exit(-1);
    }

    /* STORING THE DATA */
    AVFrame *pFrame = NULL;    // the place to actually store the frame
    pFrame = av_frame_alloc(); // allocate video frame
    if (!pFrame)
    {
        std::cout<<"Couldn't allocate AVFrame"<<std::endl;
        exit(-1);
    }

    AVPacket *pPacket = NULL;
    pPacket = av_packet_alloc();
    if (!pPacket)
    {
        std::cout<<"Couldn't allocate AVPacket"<<std::endl;
        exit(-1);
    }


}