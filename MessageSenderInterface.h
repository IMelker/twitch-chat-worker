//
// Created by l2pic on 04.04.2021.
//

#ifndef CHATCONTROLLER__MESSAGESENDERINTERFACE_H_
#define CHATCONTROLLER__MESSAGESENDERINTERFACE_H_

#include <string>

struct MessageSenderInterface {
    virtual void sendMessage(const std::string &account, const std::string &channel, const std::string &text) = 0;
};

#endif //CHATCONTROLLER__MESSAGESENDERINTERFACE_H_
