#ifndef SCREEN_RECORDER_SETTINGSCONF_H
#define SCREEN_RECORDER_SETTINGSCONF_H

#include <iostream>
#include <algorithm>

/*
 * vofs = video only fullscreen
 * avfs = audio and video fullscreen
 * vosp = video only screen portion
 * avsp = audio video screen portion
 */
enum Command{
    stop, vofs, avfs, vosp, avsp
};

class SettingsConf {

};

void welcome_msg();
Command options_menu();
void show_audio_video_options();
void show_screen_options();
static int get_answer();
bool valid_answer(std::string &answer);
bool valid_answer(std::string &answer, bool &res);
static bool yes_no_question(std::string const &message);

#endif //SCREEN_RECORDER_SETTINGSCONF_H


