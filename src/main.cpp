#include <stdio.h>
#include <iostream>

#include "wrappers/wrappers.h"

int main(int argc, char **argv) {
    init();
    FormatContext v_inFmtCtx = openInput(":0.0", "x11grab", nullptr);
    FormatContext a_inFmtCtx = openInput("default", "pulse", nullptr);

    int res;
    res = prepareDecoder(v_inFmtCtx, AVMEDIA_TYPE_VIDEO);
    res = prepareDecoder(a_inFmtCtx, AVMEDIA_TYPE_AUDIO);

    FormatContext outFmtCtx = openOutput("screen_rec.mp4");

    res = prepareEncoder(&v_inFmtCtx, &outFmtCtx, AVMEDIA_TYPE_VIDEO);
    res = prepareEncoder(&a_inFmtCtx, &outFmtCtx, AVMEDIA_TYPE_AUDIO);

    res = writeHeader(outFmtCtx);
    decode(v_inFmtCtx, outFmtCtx, AVMEDIA_TYPE_VIDEO);
    res = writeTrailer(outFmtCtx);
}