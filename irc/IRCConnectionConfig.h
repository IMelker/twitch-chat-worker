//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCCONNECTIONCONFIG_H_
#define CHATCONTROLLER_IRC_IRCCONNECTIONCONFIG_H_

#include <string>

struct IRCConnectionConfig {
    std::string host;
    int port = 6667;
    int threads = 1;
    int connect_attemps_limit = 30;
};

#endif //CHATCONTROLLER_IRC_IRCCONNECTIONCONFIG_H_
