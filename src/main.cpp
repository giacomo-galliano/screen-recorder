#include <stdio.h>
#include <iostream>

#include "ScreenRecorder.h"
#include "wrappers/wrappers.h"

int main(int argc, char **argv) {

    ScreenRecorder sr;

    SettingsConf sc;
    sr.rec_type = sc.optionsMenu();

    sr.open_();
    sr.start_();


    return 0;
}