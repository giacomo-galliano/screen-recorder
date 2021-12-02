#include "SettingsConf.h"

void welcomeMsg(){
    std::cout << R"(
     ____                       ___                      __
    / __/__________ ___ ___    / _ \___ _______  _______/ /__ ____
   _\ \/ __/ __/ -_) -_) _ \  / , _/ -_) __/ _ \/ __/ _  / -_) __/
  /___/\__/_/  \__/\__/_//_/ /_/|_|\__/\__/\___/_/  \_,_/\__/_/
    )" << std::endl;
}

Command SettingsConf::options_menu(){
    unsigned short res;
    while(true){
        show_audio_video_options();
        res = get_answer();
        switch(res){
            case 0:
                if(yes_no_question("Are you sure you want to quit?")){
                    std::cout << "Closing screen recorder.." << std::endl;
                    return Command::stop;
                }
                break;
            case 1:
                show_screen_options();
                res = get_answer();
                switch(res){
                    case 0:
                        if(yes_no_question("Are you sure you want to quit?")){
                            std::cout << "Closing screen recorder.." << std::endl;
                            return Command::stop;
                        }
                        break;
                    case 1:
                        return Command::vofs;
                    case 2:
                        return Command::vosp;
                    default:
                        std::cout << "Command not recognized" << std::endl;
                }
                break;
            case 2:
                show_screen_options();
                res = get_answer();
                switch(res){
                    case 0:
                        if(yes_no_question("Are you sure you want to quit?")){
                            std::cout << "Closing screen recorder.." << std::endl;
                            return Command::stop;
                        }
                        break;
                    case 1:
                        return Command::avfs;
                    case 2:
                        return Command::avsp;
                    default:
                        std::cout << "Command not recognized" << std::endl;
                }
                break;
            default:
                std::cout << "Command not recognized" << std::endl;
        }
    }
}

void SettingsConf::show_audio_video_options(){
    std::cout << "Which type of recording do you want to perform?\n"
              << "\t1. Record only video\n"
              << "\t2. Record both audio and video\n"
              << "\tq. Exit\n\n"
              << ">> ";
}

void SettingsConf::show_screen_options(){
    std::cout << "Which portion of the screen do you want to record?\n"
              << "\t1. Record fullscreen\n"
              << "\t2. Record only a portion of the screen\n"
              << "\tq. Exit\n\n"
              << ">> ";
}

bool SettingsConf::valid_answer(std::string &answer){
    std::transform(answer.begin(), answer.end(), answer.begin(), [](char c){return tolower(c);});

    bool valid_ans = (answer == "q") || (answer == "quit") || (answer == "exit");

    return valid_ans;
}

bool SettingsConf::valid_answer(std::string &answer, bool &res){
    std::transform(answer.begin(), answer.end(), answer.begin(), [](char c){return tolower(c);});

    bool valid_ans = (answer == "y") || (answer == "yes") ||
                     (answer == "n") || (answer == "no");
    res = valid_ans && (answer[0] == 'y');
    return valid_ans;
}

bool SettingsConf::yes_no_question(std::string const &message){
    std::string user_answer;
    bool res;

    std::cout << message << " [y/n] ";
    while(std::cin >> user_answer && !valid_answer(user_answer, res)){
        std::cout << "Invalid answer, retry.\n" << message << " [y/n] ";
    }

    if(!std::cin){
        //throw std::runtime_error("Failed to read user input");
    }

    return res; //true if "yes", false if "no"
}

int SettingsConf::get_answer(){
    std::string user_answer;

    while(std::cin >> user_answer && (!valid_answer(user_answer) && (user_answer!="1" && user_answer!="2"))){
        std::cout << "Invalid answer " << user_answer << "\nTry again.." << std::endl;
    }
    if(!std::cin){
        //throw std::runtime_error("Failed to read user input");
    }
    if(std::isdigit(user_answer[0])) return std::stoi(user_answer);
    return 0;
};
