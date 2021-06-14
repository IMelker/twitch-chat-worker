//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_EVENTS_BOTTIMEREVENT_H_
#define CHATCONTROLLER_BOT_EVENTS_BOTTIMEREVENT_H_

class BotTimerEvent : public BotEvent {
  public:
    BotTimerEvent() : BotEvent(BotEventType::Timer) {

    }
    ~BotTimerEvent() = default;

  private:

};

#endif //CHATCONTROLLER_BOT_EVENTS_BOTTIMEREVENT_H_
