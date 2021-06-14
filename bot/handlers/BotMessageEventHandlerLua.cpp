//
// Created by l2pic on 03.06.2021.
//

#include <so_5/send_functions.hpp>
#include <nlohmann/json.hpp>

#include "Clock.h"

#include "BotMessageEventHandlerLua.h"
#include "../BotEvents.h"
#include "../BotEngine.h"

using json = nlohmann::json;

int my_exception_handler(lua_State* L,
                         sol::optional<const std::exception&> maybe_exception,
                         sol::string_view description) {
    // L is the lua state, which you can wrap in a state_view if
    // necessary maybe_exception will contain exception, if it
    // exists description will either be the what() of the
    // exception or a description saying that we hit the
    // general-case catch(...)
    std::cout << "An exception occurred in a function, here's "
                 "what it says ";
    if (maybe_exception) {
        std::cout << "(straight from the exception): ";
        const std::exception& ex = *maybe_exception;
        std::cout << ex.what() << std::endl;
    }
    else {
        std::cout << "(from the description parameter): ";
        std::cout.write(description.data(),
                        static_cast<std::streamsize>(description.size()));
        std::cout << std::endl;
    }

    // you must push 1 element onto the stack to be
    // transported through as the error object in Lua
    // note that Lua -- and 99.5% of all Lua users and libraries
    // -- expects a string so we push a single string (in our
    // case, the description of the error)
    return sol::stack::push(L, description);
}

BotMessageEventHandlerLua::BotMessageEventHandlerLua(BotEngine *bot, int id, const string &script, const string &additional)
  : BotMessageEventHandler(bot, id, HandlerType::Lua), script(script), text(""), user("") {
    json add = json::parse(additional, nullptr, false, true);
    valid = !add.is_discarded();

    if (valid) {
        pcrecpp::RE_Options options;
        options.set_utf8(true);
        auto &route = add["route"];
        if (!route.is_null()) {
            auto &t = route["text"];
            if (t.is_string())
                text = pcrecpp::RE(t.get<std::string>(), options);
            auto &u = route["user"];
            if (u.is_string())
                user = pcrecpp::RE(u.get<std::string>(), options);
        }
    }

    initLuaState(lua);
}

BotMessageEventHandlerLua::~BotMessageEventHandlerLua() = default;

void BotMessageEventHandlerLua::initLuaState(sol::state &lua) {
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::utf8, sol::lib::math);

    //lua.set_exception_handler(&my_exception_handler);

    auto message_type = lua.new_usertype<Chat::Message>("message");
    message_type.set("user", sol::readonly(&Chat::Message::user));
    message_type.set("channel", sol::readonly(&Chat::Message::channel));
    message_type.set("text", sol::readonly(&Chat::Message::text));
    message_type.set("lang", sol::readonly(&Chat::Message::lang));
    message_type.set("timestamp", sol::readonly(&Chat::Message::timestamp));
    message_type.set("valid", sol::readonly(&Chat::Message::valid));
}

void BotMessageEventHandlerLua::handleBotMessage(const BotMessageEvent &evt) {
    const auto & message = evt.getMessage();
    if (!match(*message))
        return;

    const auto & config = bot->getConfig();

    auto messageToLua = [this, &message] (sol::state& lua) -> bool {
        auto engine = lua.create_named_table("engine");
        engine["message"] = *message;

        engine.set_function("log", [this, &lua, &message] (const std::string& text) {
            so_5::send<Bot::LogMessage>(this->bot->getBotLogger(),
                                        this->bot->getConfig().userId,
                                        this->bot->getConfig().botId,
                                        this->getId(), message->uuid.first,
                                        CurrentTime<std::chrono::system_clock>::milliseconds(), text);
        });
        engine.set_function("send", [this,
            &account = this->bot->getConfig().account,
            &channel = this->bot->getConfig().channel] (const std::string& text) {
            so_5::send<Chat::SendMessage>(this->bot->getMsgSender(), account, channel, text);
        });
        return true;
    };

    bool inited = messageToLua(lua);
    sol::protected_function_result pfr = lua.safe_script(script, &sol::script_pass_on_error);
    if (inited && !pfr.valid()) {
        sol::error err = pfr;
        so_5::send<Bot::LogMessage>(this->bot->getBotLogger(), config.userId, config.botId, getId(), message->uuid.first,
                                    CurrentTime<std::chrono::system_clock>::milliseconds(), err.what());
    }
}

bool BotMessageEventHandlerLua::match(const Chat::Message &msg) const {
    if (!valid)
        return false;

    if (!text.pattern().empty()) {
        if (!text.error().empty() || !text.PartialMatch(msg.text))
            return false;
    }
    if (!user.pattern().empty()) {
        if (!user.error().empty() || !user.PartialMatch(msg.user))
            return false;
    }
    return true;
};
