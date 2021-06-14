//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLERCOMMAND_H_
#define CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLERCOMMAND_H_

#include <string>

#include "BotMessageEventHandler.h"

class BotMessageEventHandlerCommand : public BotMessageEventHandler
{
  public:
    BotMessageEventHandlerCommand(BotEngine * bot, int id, const std::string& text, const std::string& additional);
    ~BotMessageEventHandlerCommand() override;

    // BotMessageEventHandler implementation
    void handleBotMessage(const BotMessageEvent& evt) override;
  private:
    std::string command;
    std::string user;
    std::string text;
    bool mention = false;
};

#endif //CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLERCOMMAND_H_
