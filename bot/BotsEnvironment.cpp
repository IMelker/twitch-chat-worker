//
// Created by imelker on 02.04.2021.
//

#include <nlohmann/json.hpp>

#include "../common/ThreadPool.h"
#include "../common/Logger.h"
#include "../DBController.h"
#include "BotsEnvironment.h"
#include "BotEngine.h"

using json = nlohmann::json;

BotsEnvironment::BotsEnvironment(ThreadPool *pool, DBController *db, std::shared_ptr<Logger> logger)
  : pool(pool), db(db), logger(std::move(logger)) {
    auto configs = db->loadBotConfigurations();
    for (auto &[id, config]: configs) {
        addBot(std::move(config));
    }
    this->logger->logInfo("BotsEnvironment {} bot created", configs.size());
}

BotsEnvironment::~BotsEnvironment() {
    stop();
    this->logger->logInfo("BotsEnvironment end of environment");
};

void BotsEnvironment::start() {
    std::shared_lock sl(botsMutex);
    for(auto &[id, bot]: botsById) {
        bot->start();
    }
}

void BotsEnvironment::stop() {
    std::shared_lock sl(botsMutex);
    for(auto &[id, bot]: botsById) {
        bot->stop();
    }
}

std::shared_ptr<BotEngine> BotsEnvironment::addBot(BotConfiguration config) {
    auto bot = std::make_shared<BotEngine>(config, this->logger);
    botsById.emplace(config.botId, bot);
    botsByChannel.emplace(config.channel, bot);
    return bot;
}

void BotsEnvironment::onMessage(std::shared_ptr<Message> message) {
    std::shared_lock sl(botsMutex);
    auto [start, end] = botsByChannel.equal_range(message->channel);
    for (auto it = start; it != end; ++it) {
        pool->enqueue([message = message, bot = it->second]() mutable {
            bot->handleMessage(std::move(message));
        });
    }
}

std::tuple<int, std::string> BotsEnvironment::processHttpRequest(std::string_view path, const std::string &request,
                                                                 std::string &error) {
    if (path == "reloadconfiguration")
        return {200, handleReloadConfiguration(request, error)};

    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}

void BotsEnvironment::setSender(MessageSenderInterface *sender) {
    irc = sender;

    std::shared_lock sl(botsMutex);
    for (auto &[id, bot]: botsById) {
        bot->setSender(sender);
    }
}


void BotsEnvironment::setBotLogger(BotLogger *blogger) {
    botLogger = blogger;

    std::shared_lock sl(botsMutex);
    for (auto &[id, bot]: botsById) {
        bot->setBotLogger(botLogger);
    }
}

std::string BotsEnvironment::handleReloadConfiguration(const std::string &, std::string &error) {
    auto configs = db->loadBotConfigurations();
    if (configs.empty()) {
        error = "Failed to load configurations";
        return "";
    }

    json body = json::object();

    std::shared_lock sl(botsMutex);
    for (auto &[id, config]: configs) {
        auto it = botsById.find(id);
        if (it == botsById.end()) {
            sl.unlock();
            {
                std::unique_lock ul(botsMutex);
                auto bot = addBot(config);
                bot->setSender(irc);
                bot->setBotLogger(botLogger);
                bot->start();
            }
            sl.lock();

            body[std::to_string(config.botId)] = "created";
        } else {
            it->second->stop();
            it->second->setConfig(config);
            it->second->start();

            body[std::to_string(config.botId)] = "updated";
        }
    }

    auto result = body.dump();
    this->logger->logInfo("BotsEnvironment bots configuration reloaded: {}", result);
    return result;
}

