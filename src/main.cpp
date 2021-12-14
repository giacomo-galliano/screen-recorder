#include <stdio.h>
#include <iostream>

#include "ScreenRecorder.h"
#include "wrappers/wrappers.h"

int main(int argc, char **argv) {


    ScreenRecorder* sr = new ScreenRecorder();

        SettingsConf sc;
        sr->rec_type = sc.optionsMenu();

        sr->open_();
        sr->start_();

        return 0;
    /*
    init();
    FormatContext v_inFmtCtx = openInput(":0.0", "x11grab", nullptr);
    FormatContext a_inFmtCtx = openInput("default", "pulse", nullptr);

    int res;
    res = prepareDecoder(v_inFmtCtx, AVMEDIA_TYPE_VIDEO);
    res = prepareDecoder(a_inFmtCtx, AVMEDIA_TYPE_AUDIO);

    char* out_filename = "../media/screen_rec.mp4";
    FormatContext outFmtCtx = openOutput(out_filename);

    res = prepareEncoder(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
    res = prepareEncoder(a_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_AUDIO);

    res = writeHeader(outFmtCtx);
    decode(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
    decode(a_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_AUDIO);
    res = writeTrailer(outFmtCtx);

     */
}

/*
 * #include <iostream>
#include <thread>
#include <string>
#include <chrono>

static bool recording = true;

void print(){
    std::string msg = "Recording";

    while(recording == true){
        std::cout << "\r\033[1;31m" << msg << "\033[0m";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "\r            ";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

-----------
        for(int i=0; i<msg.length(); i++){
            std::cout << "\b";
        }
---------

}
}

int main()
{
    std::thread printer(print);
    std::cin.get();

    recording = false;
    printer.join();
    std::cout << "\nRecording stopped" << std::endl;

    std::cin.get();

    return 0;
}
 */