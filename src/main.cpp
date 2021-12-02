#include <stdio.h>
#include <iostream>

#include "ScreenRecorder.h"
#include "wrappers/wrappers.h"

int main(int argc, char **argv) {

    /*
    ScreenRecorder* sr;
    try{
        SettingsConf sc;
        Command cmd = sc.options_menu();
        sr.start(cmd);
    }
    */
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
}