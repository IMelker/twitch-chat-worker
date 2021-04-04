//
// Created by l2pic on 03.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTCONFIGURATION_H_
#define CHATSNIFFER_BOT_BOTCONFIGURATION_H_

#include <string>
#include <vector>

enum class BotEventType : int {
    Message = 1
};

struct BotConfiguration {
    int botId = 0;
    std::string channel;

    std::vector<std::string> onMessage;
};

inline std::vector<std::string>& getHandlers(int eventTypeId, BotConfiguration& config) {
    if (static_cast<BotEventType>(eventTypeId) == BotEventType::Message) {
        return config.onMessage;
    }
    assert(false && "can't be there");
}

#endif //CHATSNIFFER_BOT_BOTCONFIGURATION_H_
