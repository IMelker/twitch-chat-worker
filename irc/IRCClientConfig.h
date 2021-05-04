//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCCLIENTCONFIG_H_
#define CHATCONTROLLER_IRC_IRCCLIENTCONFIG_H_

#include <string>

struct IRCClientConfig {
    int id = 0;
    std::string nick;
    std::string user;
    std::string password;
    int channels_limit = 20;
    int command_per_sec_limit = 20 / 30;
    int whisper_per_sec_limit = 3;
    int auth_per_sec_limit = 2;
    int session_count = 1;
};

#endif //CHATCONTROLLER_IRC_IRCCLIENTCONFIG_H_
