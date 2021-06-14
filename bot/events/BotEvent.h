//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_BOTEVENT_H_
#define CHATCONTROLLER_BOT_BOTEVENT_H_

#include <string>

// Base class for events
class BotEvent {
  protected:
    enum class BotEventType {
        Message,
        Timer,
        Request,
        Unknown
    };
    friend std::string toString(BotEventType type) {
        switch (type) {
            case BotEventType::Message:
                return "Message";
            case BotEventType::Timer:
                return "Timer";
            case BotEventType::Request:
                return "Request";
            case BotEventType::Unknown:
            default:
                return "Unknown";
        }
    }

  public:
    explicit BotEvent(BotEventType type) : type(type), name(toString(type)) {
    }
    ~BotEvent() = default;

  private:
    BotEventType type = BotEventType::Unknown;
    std::string name;
};

#endif //CHATCONTROLLER_BOT_BOTEVENT_H_
