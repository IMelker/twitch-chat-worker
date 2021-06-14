//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLER_H_
#define CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLER_H_

#include "../events/BotMessageEvent.h"
#include "BotEventHandler.h"

class BotMessageEventHandler : public BotEventHandler
{
  public:
    explicit BotMessageEventHandler(BotEngine * bot, int id, HandlerType type) : BotEventHandler(bot, id, type) {}
    ~BotMessageEventHandler() override = default;

    virtual void handleBotMessage(const BotMessageEvent& evt) = 0;
};

#endif //CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLER_H_
