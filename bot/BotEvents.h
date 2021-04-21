//
// Created by l2pic on 19.04.2021.
//

#ifndef CHATCONTROLLER_BOT_BOTEVENTS_H_
#define CHATCONTROLLER_BOT_BOTEVENTS_H_

#include <string>
#include "BotConfiguration.h"
#include "../common/Utils.h"

namespace Bot {

struct Shutdown final : public so_5::signal_t {};

struct Reload {
    mutable BotConfiguration config;
};

struct LogMessage {
    int userId = 0;
    int botId = 0;
    int handlerId = 0;
    uint128_t messageId;
    long long timestamp = 0;
    std::string text;
};

}

#endif //CHATCONTROLLER_BOT_BOTEVENTS_H_
