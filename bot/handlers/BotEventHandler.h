//
// Created by l2pic on 03.06.2021.
//

#ifndef CHATCONTROLLER_BOT_BOTEVENTHANDLER_H_
#define CHATCONTROLLER_BOT_BOTEVENTHANDLER_H_

#include <so_5/agent.hpp>

#include "../BotConfiguration.h"

class BotEngine;
class BotEventHandler
{
  public:
    enum class HandlerType {
        Lua = 1,
        Command = 2,
        Unknown = 0
    };

  public:
    explicit BotEventHandler(BotEngine * bot, int id, HandlerType type) : id(id), bot(bot), type(type) {};
    virtual ~BotEventHandler() = default;

    [[nodiscard]] int getId() const { return id; };

  protected:
    int id = 0; // id from database
    bool valid = true;
    BotEngine *bot = nullptr;
    HandlerType type = HandlerType::Unknown;
};

#endif //CHATCONTROLLER_BOT_BOTEVENTHANDLER_H_
