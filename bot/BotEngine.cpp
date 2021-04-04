//
// Created by imelker on 02.04.2021.
//

#include <sol/sol.hpp>

#include "../irc/IRCWorkerPool.h"
#include "../common/Logger.h"
#include "BotEngine.h"

BotEngine::BotEngine(BotConfiguration config, IRCWorkerPool *irc, std::shared_ptr<Logger> logger)
  : irc(irc), logger(std::move(logger)), config(std::move(config)) {

}

BotEngine::~BotEngine() = default;

int BotEngine::getId() const {
    return config.botId;
}

void BotEngine::updateConfiguration(BotConfiguration cfg) {
    this->config = std::move(cfg);
}

void BotEngine::handleMessage(const Message &message) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::utf8, sol::lib::math);

    auto message_type = lua.new_usertype<Message>("message");
    message_type.set("user", sol::readonly(&Message::user));
    message_type.set("channel", sol::readonly(&Message::channel));
    message_type.set("text", sol::readonly(&Message::text));
    message_type.set("lang", sol::readonly(&Message::lang));
    message_type.set("timestamp", sol::readonly(&Message::timestamp));
    message_type.set("valid", sol::readonly(&Message::valid));

    auto engine = lua.create_named_table("engine");
    engine["message"] = message;
    engine.set_function("send", [irc = this->irc, &channel = message.channel] (const std::string& text) {
        irc->sendMessage(channel, text);
    });

    for (auto &script: config.onMessage) {
        auto res = lua.script(script);
        if (res.status() != sol::call_status::ok) {
            abort();
        }
    }
}
