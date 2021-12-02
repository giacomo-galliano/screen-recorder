#ifndef SCREEN_RECORDER_SCREENRECORDER_H
#define SCREEN_RECORDER_SCREENRECORDER_H

#define OUT_VIDEO_INDEX 0
#define OUT_AUDIO_INDEX 1

#include "SettingsConf.h"

class ScreenRecorder {
int in_v_index, in_a_index;

public:
    void start(Command cmd);
};

#endif //SCREEN_RECORDER_SCREENRECORDER_H
