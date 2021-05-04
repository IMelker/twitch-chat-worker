//
// Created by l2pic on 03.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTCONFIGURATION_H_
#define CHATSNIFFER_BOT_BOTCONFIGURATION_H_

#include <cassert>
#include <string>
#include <vector>

using Handler = std::tuple<std::string, int, std::string>;
using BotHandlers = std::vector<std::tuple<std::string, int, std::string>>;

enum class BotEventType : int {
    Message = 1,
    Interval = 2,
    Timer = 3
};

struct BotConfiguration {
    int botId = 0;
    int userId = 0;
    std::string account;
    std::string channel;

    BotHandlers onMessage;
    BotHandlers onInterval;
    BotHandlers onTimer;
};

inline BotHandlers& getHandlers(int eventTypeId, BotConfiguration& config) {
    if (static_cast<BotEventType>(eventTypeId) == BotEventType::Message) {
        return config.onMessage;
    }
    if (static_cast<BotEventType>(eventTypeId) == BotEventType::Interval) {
        return config.onInterval;
    }
    if (static_cast<BotEventType>(eventTypeId) == BotEventType::Timer) {
        return config.onTimer;
    }
    assert(false && "can't be there");
}

#endif //CHATSNIFFER_BOT_BOTCONFIGURATION_H_
