#include <stdio.h>
#include <iostream>
#include "ScreenRecorder.h"

int main(int argc, char **argv) {

ScreenRecorder* sc = new ScreenRecorder;

    sc->openInput();
    sc->PrepareDecoder();
    sc->prepareVideoEncoder();
    sc->openOutput();
    sc->writeHeader();
    sc->decoding();
    sc->writeTrailer();


return 0;

}