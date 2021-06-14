//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLERLUA_H_
#define CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLERLUA_H_

#include <pcrecpp.h>
#include <sol/sol.hpp>

#include "BotMessageEventHandler.h"

class BotMessageEventHandlerLua : public BotMessageEventHandler
{
  public:
    BotMessageEventHandlerLua(BotEngine *bot, int id, const std::string& text, const std::string& additional);
    ~BotMessageEventHandlerLua() override;

    // BotMessageEventHandler implementation
    void handleBotMessage(const BotMessageEvent& evt) override;
  private:
    void initLuaState(sol::state& lua);

    [[nodiscard]] bool match(const Chat::Message& msg) const;

    sol::state lua;

    std::string script;

    pcrecpp::RE text;
    pcrecpp::RE user;
};

#endif //CHATCONTROLLER_BOT_EVENTS_BOTMESSAGEEVENTHANDLERLUA_H_
