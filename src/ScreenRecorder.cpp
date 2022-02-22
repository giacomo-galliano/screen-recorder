#include "ScreenRecorder.h"
#include <malloc.h>

ScreenRecorder::ScreenRecorder() : status(RecStatus::RECORDING){
    avdevice_register_all();
//    avformat_network_init();
    getFilenameOut(outFileName);
    outFmtCtx = avformat_alloc_context();
    outFmt = av_guess_format(nullptr, outFileName.c_str(), nullptr);
    if (!outFmt) {
        throw runtime_error{"Cannot guess format"};
    }
    avformat_alloc_output_context2(&outFmtCtx, outFmt, outFmt->name, outFileName.c_str());

    vPTS = 0;
    aPTS = 0;
}
ScreenRecorder::~ScreenRecorder() {
    readVideoThread->join();
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
    writeTrailer();
    avformat_close_input(&inVFmtCtx);
    avformat_free_context(inVFmtCtx);
}


void ScreenRecorder::open_(){
    switch(rec_type){
        case Command::vofs:
            openVideoInput();
            break;
        case Command::avfs:
//            v_inFmtCtx = openInput(AVMEDIA_TYPE_VIDEO);
//            a_inFmtCtx = openInput(AVMEDIA_TYPE_AUDIO);
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
}

void ScreenRecorder::start_(){
    switch(rec_type){
        case Command::vofs:
            initVideoEncoder();
            writeHeader();
            readVideoThread = new std::thread([this](){
                this->readFrame();
            });

            videoThread = new std::thread([this](){
                this->processVideo();
            });

            break;
        case Command::avfs:
//            prepareDecoder(v_inFmtCtx, AVMEDIA_TYPE_VIDEO);
//            prepareDecoder(a_inFmtCtx, AVMEDIA_TYPE_AUDIO);
//            prepareEncoder(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
//            prepareEncoder(a_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_AUDIO);
//            writeHeader(outFmtCtx);
//            videoThread = new std::thread([this](){
//                this->recording.store(true);
//                this->writing.store(false);
//                this->finished.store(false);
////                this->decode(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
//            });
////            std::this_thread::sleep_for(std::chrono::seconds(2));
//            audioThread = new std::thread([this](){
//                this->decode(a_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_AUDIO);
//            });
//            std::cout << "Recording.." << std::endl;
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

    std::cout << "Recording.." << std::endl;

    PSRMenu();

}

void ScreenRecorder::pause_(){
    lock_guard<mutex> ul(status_lock);
    status = RecStatus::PAUSE;
    malloc_trim(0);
    std::cout << "Recording paused.." << std::endl;
}

void ScreenRecorder::restart_(){
    lock_guard<mutex> lg(status_lock);
    status = RecStatus::RECORDING; //status = RecStatus::RESTARTING; //TODO: ---come usare restarting per risistemare i pts7cv.notify_all();
    cv.notify_all();

    std::cout << "Recording.." << std::endl;
}

void ScreenRecorder::stop_(){
    lock_guard<mutex> lg(status_lock);
    status = RecStatus::STOP;
    cv.notify_all();

    std::cout << "Recording stopped." << std::endl;

}


void ScreenRecorder::writeHeader(){
    if(avio_open2(&outFmtCtx->pb, outFileName.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr) < 0){
        throw runtime_error{"Could not open out file."};
    }
    if (avformat_write_header(outFmtCtx, nullptr) < 0) {
        throw runtime_error{"Could not write header."};
    }

}

void ScreenRecorder::writeTrailer(){
    if(av_write_trailer(outFmtCtx) < 0) {
        throw runtime_error{"Could not write trailer."};
    }

    if(avio_close(outFmtCtx->pb) < 0){
        throw runtime_error{"Could not close file."};
    }

}

void ScreenRecorder::openVideoInput() {

    sourceOptions = nullptr;
    inVFmtCtx = avformat_alloc_context();

#ifdef _WIN32
    ift = av_find_input_format("gdigrab");
    if (avformat_open_input(&inFmtCtx, "desktop", ift, &options) != 0) {
        cerr << "Couldn't open input stream" << endl;
        exit(-1);
    }

#elif defined linux

//        char *displayName = getenv("DISPLAY");
//    av_dict_set (&sourceOptions, "follow_mouse", "centered", 0);
    av_dict_set (&sourceOptions, "framerate", "30", 0);
//    av_dict_set (&sourceOptions, "video_size", "wxga", 0);//wxga==1366x768
    av_dict_set (&sourceOptions, "probesize", "40M", 0);
    //TODO: capire a che valore settare -> [4 * width * height * 2 + 1] (e se effetivamente serve)

//    inVFmtCtx->probesize = 40000000;
    int offset_x = 0, offset_y = 0;
    string url = ":0.0+" + std::to_string(offset_x) + "," + std::to_string(offset_y);  //custom string to set the start point of the screen section
    inVFmt = av_find_input_format("x11grab");
    if (inVFmt == nullptr) {
        throw logic_error{"av_find_input_format not found..."};
    }
    if( avformat_open_input(&inVFmtCtx, url.c_str(), inVFmt, &sourceOptions) != 0) {
        throw runtime_error{"cannot open video device"};
    }

#else

    res = av_dict_set(&options, "pixel_format", "0rgb", 0);
    if (res < 0) {
        cerr << "Error in setting pixel format" << endl;
        exit(-1);
    }

    res = av_dict_set(&options, "video_device_index", "1", 0);

    if (res < 0) {
        cerr << "Error in setting video device index" << endl;
        exit(-1);
    }

    ift = av_find_input_format("avfoundation");

    if (avformat_open_input(&inFmtCtx, "Capture screen 0:none", ift, &options) != 0) {  //TODO trovare un modo per selezionare sempre lo schermo (forse "Capture screen 0")
        cerr << "Error in opening input device" << endl;
        exit(-1);
    }

#endif

    if(avformat_find_stream_info(inVFmtCtx, &sourceOptions) < 0 ){
        throw logic_error{"cannot find correct stream info..."};
    }

    for(int i=0; i < inVFmtCtx->nb_streams; i++){
        if (inVFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            inVIndex = i;
            break;
        }
    }
    if (inVIndex == -1 || inVIndex >= (int)inVFmtCtx->nb_streams) {
        throw logic_error{"Didn't find a video stream."};
    }

    inVC = avcodec_find_decoder(inVFmtCtx->streams[inVIndex]->codecpar->codec_id);
    if(!inVC){
        throw logic_error{"Decoder codec not found."};
    }

    inVCCtx = avcodec_alloc_context3(inVC);
    if(!inVCCtx){
        throw runtime_error{"Could not allocate video contex"};
    }
    if(avcodec_parameters_to_context(inVCCtx, inVFmtCtx->streams[inVIndex]->codecpar) < 0){
        throw runtime_error{"Video parameter to contex error"};
    }

    if(avcodec_open2(inVCCtx, inVC, nullptr) < 0){
        throw runtime_error{"Could not open decoder."};
    }
}

void ScreenRecorder::initVideoEncoder(){

    outVC = avcodec_find_encoder(AV_CODEC_ID_H264); //TODO: scegliere tra H264 e MPEG4
    if(!outVC){
        throw logic_error{"Encoder codec not found"};
    }

    outVStream = avformat_new_stream(outFmtCtx, outVC);
//    outVStream->index = OUT_VIDEO_INDEX;
    if(!outVStream){
        throw runtime_error{"Could not create out video stream."};
    }

    outVCCtx = avcodec_alloc_context3(outVC);
    if(!outVCCtx){
        throw runtime_error{"Could not allocate video encoder contex"};
    }

//    avcodec_parameters_to_context(outVCCtx, outVStream->codecpar);

    outVCCtx->width = inVCCtx->width;
    outVCCtx->height = inVCCtx->height;
    outVCCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    outVCCtx->time_base = (AVRational){1,60};
    outVCCtx->framerate = (AVRational){15,1};//av_inv_q(cCtx->time_base);
    outVCCtx->pix_fmt = AV_PIX_FMT_YUV420P;
//    outVCCtx->bit_rate = 4000;
//        cCtx->gop_size = 10;
//        cCtx->max_b_frames = 0;
//        cCtx->profile = FF_PROFILE_H264_MAIN;
    if ( outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        outVCCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

//    av_opt_set(outVCCtx, "preset", "ultrafast", 0);


    if(avcodec_open2(outVCCtx, outVC, nullptr) < 0){
        throw runtime_error{"Failed to open video encoder."};
    }

    if(avcodec_parameters_from_context(outFmtCtx->streams[OUT_VIDEO_INDEX]->codecpar, outVCCtx) < 0){
        throw runtime_error{"Video parameter from contex error"};
    }

    swsCtx = sws_getContext(
            inVCCtx->width,
            inVCCtx->height,
            inVCCtx->pix_fmt,
            outVCCtx->width,
            outVCCtx->height,
            outVCCtx->pix_fmt,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);


}

void ScreenRecorder::readFrame(){
    AVPacket* pkt;

    while(true) {
        unique_lock<mutex> ul(status_lock);
        if (status == RecStatus::STOP){
            break;
        }
        cv.wait(ul, [this]() { return status != RecStatus::PAUSE; });
        ul.unlock();

        pkt = av_packet_alloc();
        if (av_read_frame(inVFmtCtx, pkt) < 0) {
            throw std::runtime_error("Error in getting RawPacket");
        }

        unique_lock<mutex> video_queue_ul{video_queue_mutex};
        video_queue.push_back(pkt);
        video_queue_ul.unlock();

    }
}

void ScreenRecorder::processVideo() {

    AVPacket *inPkt;

    AVPacket outPkt;
    av_init_packet(&outPkt);

    AVFrame* frame = av_frame_alloc();;

    AVFrame *convFrame = av_frame_alloc();;
//    if(!convFrame){
//        throw runtime_error{"Could not allocate convFrame."};
//    }

//    uint8_t *buffer = new uint8_t[av_image_get_buffer_size( outVCCtx->pix_fmt,outVCCtx->width,outVCCtx->height,1)];
    uint8_t *buffer = (uint8_t *) av_malloc(
            av_image_get_buffer_size( outVCCtx->pix_fmt,outVCCtx->width,outVCCtx->height,1));
//    if (buffer == NULL) {
//        throw runtime_error{"Could not allocate image buffer."};
//    }

//  av_image_alloc(convFrame->data, convFrame->linesize, outVCCtx->width,
//                                   outVCCtx->height, outVCCtx->pix_fmt, 1);
    if((av_image_fill_arrays(convFrame->data, convFrame->linesize,
                             buffer,
                             outVCCtx->pix_fmt,
                             outVCCtx->width,
                             outVCCtx->height, 1)) <0){
        throw runtime_error{"Could not fill arrays."};
    }

    int res = 0;

    while(true){

        unique_lock<mutex> video_queue_ul{video_queue_mutex};
        if(!video_queue.empty()) {
            inPkt = video_queue.front();
            video_queue.pop_front();
//            video_queue.shrink_to_fit();
            video_queue_ul.unlock();

            if(inPkt->stream_index == inVIndex){

                inPkt->pts =  vPTS++ * outFmtCtx->streams[OUT_VIDEO_INDEX]->time_base.den / outVCCtx->framerate.num;

                res = avcodec_send_packet(inVCCtx, inPkt);
                av_packet_unref(inPkt);
                av_packet_free(&inPkt);
                if (res < 0) {
                    throw runtime_error("Decoding Error: sending packet");
                }
                if( avcodec_receive_frame(inVCCtx, frame) == 0){

                    convFrame->width = outVCCtx->width;
                    convFrame->height = outVCCtx->height;
                    convFrame->format = outVCCtx->pix_fmt;
                    av_frame_copy_props(convFrame, frame);

//                    sws_scale(swsCtx,frame->data, frame->linesize, 0,
//                              frame->height, convFrame->data, convFrame->linesize);

                    if(avcodec_send_frame(outVCCtx, convFrame) >= 0){
                        if(avcodec_receive_packet(outVCCtx, &outPkt) >= 0){
                            write_lock.lock();
                            if (av_interleaved_write_frame(outFmtCtx, &outPkt) < 0) {
                                throw runtime_error("Error in writing file");
                            }
                            write_lock.unlock();
                        }
                    }
                }else{
                    throw runtime_error("Decoding Error: receiving frame");
                }
                av_frame_unref(frame);
                av_packet_unref(&outPkt);
            }
        }else{
            video_queue.shrink_to_fit();
            video_queue.clear();
            malloc_trim(0);

            video_queue_ul.unlock();
            unique_lock<mutex> ul(status_lock);

            if (status == RecStatus::STOP)
                break;


            cv.wait(ul, [this]() { return status != RecStatus::PAUSE; });
            ul.unlock();
        }
    }
    av_packet_unref(&outPkt);
    av_frame_free(&frame);
    av_frame_free(&convFrame);

}


void ScreenRecorder::PSRMenu() {
    unsigned short res;
    while(true) {
        unique_lock<mutex> ul(status_lock);
        if(status == RecStatus::STOP)
            break;

        showPSROptions();

        ul.unlock();

        switch (getPSRAnswer()) {
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
}

void ScreenRecorder::showPSROptions(){
    if(status != RecStatus::PAUSE) {
        std::cout << "Digit \"p\" or \"pause\" to pause the recording, \"s\" or \"stop\" to terminate\n"
                  << ">> ";
    }else{
        std::cout << "Digit \"r\" or \"restart\" to resume the recording, \"s\" or \"stop\" to terminate\n"
                  << ">> ";
    }
}

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
        std::cout << "\033[1;31m" << "Invalid answer: \"" << user_answer << "\". Try again.\n" << "\033[0m" << ">> ";
    }
    if(!std::cin){
        throw std::runtime_error("Failed to read user input");
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




