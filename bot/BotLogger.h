//
// Created by l2pic on 08.04.2021.
//

#ifndef CHATCONTROLLER_BOT_BOTLOGGER_H_
#define CHATCONTROLLER_BOT_BOTLOGGER_H_

#include <string>

struct BotLogger {
    virtual void onLog(int userId, int botId, int handlerId, long long timestamp, const std::string& text) = 0;
};

#endif //CHATCONTROLLER_BOT_BOTLOGGER_H_
