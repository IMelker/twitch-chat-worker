//
// Created by l2pic on 08.04.2021.
//

#ifndef CHATCONTROLLER_BOT_BOTLOGGER_H_
#define CHATCONTROLLER_BOT_BOTLOGGER_H_

#include <string>

namespace BotLogger {

struct Message {
    int userId = 0;
    int botId = 0;
    int handlerId = 0;
    long long timestamp = 0;
    std::string text;
};

};

#endif //CHATCONTROLLER_BOT_BOTLOGGER_H_
