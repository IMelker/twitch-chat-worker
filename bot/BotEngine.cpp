//
// Created by imelker on 02.04.2021.
//

#include <sol/sol.hpp>

#include <nlohmann/json.hpp>

#include "../irc/IRCWorkerPool.h"
#include "../common/Logger.h"
#include "../common/Clock.h"
#include "../common/Timer.h"
#include "BotEngine.h"

using json = nlohmann::json;

BotEngine::BotEngine(BotConfiguration config, std::shared_ptr<Logger> logger)
  : irc(nullptr), botLogger(nullptr), logger(std::move(logger)),
    active(false), config(std::move(config)) {
    this->logger->logInfo("BotEngine bot(id={}) created on user(id={}) for channel: {}",
                           this->config.botId, this->config.userId, this->config.channel);
}

BotEngine::~BotEngine() {
    this->logger->logTrace("BotEngine bot(id={}) destroyed", this->config.botId);
};

void BotEngine::start() {
    assert(irc);
    active.store(true, std::memory_order_relaxed);

    for (auto &onTimer: config.onTimer) {
        handleTimer(onTimer);
    }

    this->logger->logInfo("BotEngine bot(id={}) started", this->config.botId);
}

void BotEngine::stop() {
    active.store(false, std::memory_order_relaxed);

    this->logger->logInfo("BotEngine bot(id={}) stoped", this->config.botId);
}

int BotEngine::getId() const {
    return config.botId;
}

void BotEngine::setConfig(BotConfiguration cfg) {
    this->logger->logInfo("BotEngine bot(id={}) config update", this->config.botId);
    this->config = std::move(cfg);
}

void BotEngine::setSender(MessageSenderInterface *sender) {
    this->irc = sender;
}

void BotEngine::setBotLogger(BotLogger *blogger) {
    this->botLogger = blogger;
}

void BotEngine::handleInterval(Handler &handler) {

}

void BotEngine::handleTimer(Handler &handler) {
    json additional = json::parse(std::get<2>(handler), nullptr, false, true);
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
            botLogger->onLog(config.userId, config.botId, lua.get<int>("_handler_id"),
                             CurrentTime<std::chrono::system_clock>::milliseconds(), text);
        });
        engine.set_function("send", [irc = this->irc,
            &account = config.account,
            &channel = config.channel] (const std::string& text) {
            irc->sendMessage(account, channel, text);
        });

        lua.set("_handler_id", id);
        auto res = lua.script(script);
        if (res.status() != sol::call_status::ok) {
            abort();
        }
    }, timestamp);
}

void BotEngine::handleMessage(std::shared_ptr<Message> message) {
    if (!active.load(std::memory_order_relaxed))
        return;

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
    engine["message"] = *message;

    engine.set_function("log", [this, &lua] (const std::string& text) {
        botLogger->onLog(config.userId, config.botId, lua.get<int>("_handler_id"),
                         CurrentTime<std::chrono::system_clock>::milliseconds(), text);
    });
    engine.set_function("send", [irc = this->irc,
                                 &account = config.account,
                                 &channel = config.channel] (const std::string& text) {
        irc->sendMessage(account, channel, text);
    });

    for (auto &[script, id, additional]: config.onMessage) {
        lua.set("_handler_id", id);
        auto res = lua.script(script);
        if (res.status() != sol::call_status::ok) {
            abort();
        }
    }
}

