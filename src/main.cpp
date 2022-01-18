#include <stdio.h>
#include <iostream>

#include "ScreenRecorder.h"

int main(int argc, char **argv) {

    ScreenRecorder* sr = new ScreenRecorder();

        SettingsConf sc;
        sr->setRecType(sc.optionsMenu());

        sr->open_();
        sr->start_();

        return 0;
}
