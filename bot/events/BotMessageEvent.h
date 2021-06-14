//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENT_H_
#define CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENT_H_

#include "BotEvent.h"
#include "../../ChatMessage.h"

class BotMessageEvent : public BotEvent {
    using MessageHolder = so_5::message_holder_t<Chat::Message>;
  public:
    explicit BotMessageEvent(MessageHolder message)
      : BotEvent(BotEventType::Message), message(std::move(message)) {
    }
    ~BotMessageEvent() = default;

    const MessageHolder& getMessage() const { return message; }

  private:
    MessageHolder message;
};

#endif //CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENT_H_
