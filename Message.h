//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER__MESSAGE_H_
#define CHATSNIFFER__MESSAGE_H_

#include <string>

struct Message {
    std::string user;
    std::string channel;
    std::string text;
    std::string lang;
    long long timestamp = 0;
    bool valid = false;
};

#endif //CHATSNIFFER__MESSAGE_H_
