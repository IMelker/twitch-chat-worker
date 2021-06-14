//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_EVENTS_BOTREQUESTEVENT_H_
#define CHATCONTROLLER_BOT_EVENTS_BOTREQUESTEVENT_H_

class BotRequestEvent : public BotEvent {
  public:
    BotRequestEvent() : BotEvent(BotEventType::Request) {

    }
    ~BotRequestEvent() = default;

  private:

};

#endif //CHATCONTROLLER_BOT_EVENTS_BOTREQUESTEVENT_H_
