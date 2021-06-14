//
// Created by l2pic on 03.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTCONFIGURATION_H_
#define CHATSNIFFER_BOT_BOTCONFIGURATION_H_

#include <cassert>
#include <string>
#include <vector>

#include "../ChatMessage.h"

class EventHandlerConfiguration {
  public:
    EventHandlerConfiguration(int id, int typeId, std::string &&text, std::string &&additional)
      : id(id), typeId(typeId), text(std::move(text)), additional(std::move(additional)) {

    }
    ~EventHandlerConfiguration() = default;

    [[nodiscard]] int getId() const { return id; }
    [[nodiscard]] int getTypeId() const { return typeId; }
    [[nodiscard]] const std::string& getText() const { return text; }
    [[nodiscard]] const std::string& getAdditional() const { return additional; }

  protected:
    int id = 0;
    int typeId = 0;
    std::string text;
    std::string additional;
};

struct BotConfiguration {
    int botId = 0;
    int userId = 0;
    std::string account;
    std::string channel;

    std::vector<EventHandlerConfiguration> handlers;
};

#endif //CHATSNIFFER_BOT_BOTCONFIGURATION_H_
