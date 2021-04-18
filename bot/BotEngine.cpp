//
// Created by imelker on 02.04.2021.
//

#include <sol/sol.hpp>

#include <nlohmann/json.hpp>

#include <so_5/send_functions.hpp>

#include "../IRCController.h"
#include "../common/Logger.h"
#include "../common/Clock.h"
#include "../common/Timer.h"
#include "BotEngine.h"

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

BotEngine::BotEngine(const context_t &ctx, so_5::mbox_t self, so_5::mbox_t msgSender, so_5::mbox_t botLogger,
                     BotConfiguration config, std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), self(std::move(self)), msgSender(std::move(msgSender)), botLogger(std::move(botLogger)),
    logger(std::move(logger)), config(std::move(config)) {

    this->logger->logInfo("BotEngine bot(id={}) created on user(id={}) for channel: {}",
                           this->config.botId, this->config.userId, this->config.channel);
}

BotEngine::~BotEngine() {
    this->logger->logTrace("BotEngine bot(id={}) destroyed", this->config.botId);
};

void BotEngine::so_define_agent() {
    so_subscribe(self).event(&BotEngine::evtChatMessage, so_5::thread_safe);
    so_subscribe_self().event(&BotEngine::evtShutdown);
    so_subscribe_self().event(&BotEngine::evtReload);
}

void BotEngine::so_evt_start() {
    for (auto &onTimer: config.onTimer) {
        handleTimer(onTimer);
    }

    this->logger->logInfo("BotEngine bot(id={}) started", this->config.botId);
}

void BotEngine::so_evt_finish() {
    this->logger->logInfo("BotEngine bot(id={}) stoped", this->config.botId);
}

void BotEngine::evtShutdown(so_5::mhood_t<BotEngine::Shutdown>) {
    so_deregister_agent_coop_normally();
}

void BotEngine::evtReload(so_5::mhood_t<BotEngine::Reload> message) {
    this->logger->logInfo("BotEngine bot(id={}) config update", this->config.botId);
    this->config = std::move(message->config);
}

void BotEngine::evtChatMessage(mhood_t<Chat::Message> message) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::utf8, sol::lib::math);

    //lua.set_exception_handler(&my_exception_handler);

    auto message_type = lua.new_usertype<Chat::Message>("message");
    message_type.set("user", sol::readonly(&Chat::Message::user));
    message_type.set("channel", sol::readonly(&Chat::Message::channel));
    message_type.set("text", sol::readonly(&Chat::Message::text));
    message_type.set("lang", sol::readonly(&Chat::Message::lang));
    message_type.set("timestamp", sol::readonly(&Chat::Message::timestamp));
    message_type.set("valid", sol::readonly(&Chat::Message::valid));

    auto engine = lua.create_named_table("engine");
    engine["message"] = *message;

    engine.set_function("log", [this, &lua, message] (const std::string& text) {
        so_5::send<BotLogger::Message>(botLogger, config.userId, config.botId,
                                       lua.get<int>("_handler_id"), message->uuid.first,
                                       CurrentTime<std::chrono::system_clock>::milliseconds(), text);
    });
    engine.set_function("send", [this, &account = config.account, &channel = config.channel] (const std::string& text) {
        so_5::send<SendMessage>(msgSender, account, channel, text);
    });

    for (auto &[script, id, additional]: config.onMessage) {
        lua.set("_handler_id", id);
        sol::protected_function_result pfr = lua.safe_script(script, &sol::script_pass_on_error);
        if (!pfr.valid()) {
            sol::error err = pfr;
            so_5::send<BotLogger::Message>(botLogger, config.userId, config.botId, id, message->uuid.first,
                                           CurrentTime<std::chrono::system_clock>::milliseconds(), err.what());
        }
    }
}

void BotEngine::handleInterval(Handler &handler) {

}

void BotEngine::handleTimer(Handler &handler) {
    /*json additional = json::parse(std::get<2>(handler), nullptr, false, true);
    if (additional.is_discarded()) {
        logger->logError("BotEngine Failed to parse timer event additional data");
        return;
    }

    auto timestamp = additional["timestamp"].get<long long>();
    Timer::instance().setTimer([this, script = std::get<0>(handler), id = std::get<1>(handler)] () {
        // TODO move away from timer thread
        if (!active.load(std::memory_order_relaxed))
            return;

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::utf8, sol::lib::math);

        auto engine = lua.create_named_table("engine");
        engine.set_function("log", [this, &lua] (const std::string& text) {
            so_5::send<BotLogger::Message>(botLogger,
                                           config.userId, config.botId, lua.get<int>("_handler_id"),
                                           CurrentTime<std::chrono::system_clock>::milliseconds(), text);
        });
        engine.set_function("send", [this, &account = config.account, &channel = config.channel] (const std::string& text) {
            so_5::send<SendMessage>(msgSender, account, channel, text);
        });

        lua.set("_handler_id", id);
        auto res = lua.script(script);
        if (res.status() != sol::call_status::ok) {
            abort();
        }
    }, timestamp);*/
}
