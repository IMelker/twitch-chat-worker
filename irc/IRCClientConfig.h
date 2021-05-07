//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCCLIENTCONFIG_H_
#define CHATCONTROLLER_IRC_IRCCLIENTCONFIG_H_

#include <string>

#include <spdlog/fmt/ostr.h>

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

    friend std::ostream& operator<<(std::ostream& os, const IRCClientConfig& c) {
        return os << "{"
                  << "id = " << c.id << ", "
                  << "nick = " << c.nick << ", "
                  << "user = " << c.user << ", "
                  << "password = " << c.password << ", "
                  << "channels_limit = " << c.channels_limit << ", "
                  << "command_per_sec_limit = " << c.command_per_sec_limit << ", "
                  << "whisper_per_sec_limit = " << c.whisper_per_sec_limit << ", "
                  << "auth_per_sec_limit = " << c.auth_per_sec_limit << ", "
                  << "session_count = " << c.session_count << " }";
    }
};

#endif //CHATCONTROLLER_IRC_IRCCLIENTCONFIG_H_
