//
// Created by imelker on 02.04.2021.
//

#include <nlohmann/json.hpp>

#include "../common/ThreadPool.h"
#include "../common/Logger.h"
#include "../DBStorage.h"
#include "BotsEnvironment.h"

using json = nlohmann::json;

BotsEnvironment::BotsEnvironment(ThreadPool *pool, DBStorage *db, std::shared_ptr<Logger> logger)
  : pool(pool), db(db), logger(std::move(logger)) {
}

BotsEnvironment::~BotsEnvironment() = default;

void BotsEnvironment::onMessage(const Message &message) {
    auto [start, end] = bots.equal_range(message.channel);
    for (auto it = start; it != end; ++it) {
        pool->enqueue([message = message, bot = it->second]() {
            bot->handleMessage(message);
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

void BotsEnvironment::initBots(IRCWorkerPool *irc) {
    assert(bots.empty());

    this->irc = irc;

    auto configs = db->loadBotConfigurations();
    for (auto &[id, config]: configs) {
        bots.emplace(config.channel, std::make_shared<BotEngine>(config, irc, this->logger));
    }
}

std::string BotsEnvironment::handleReloadConfiguration(const std::string &request, std::string &error) {
    auto configs = db->loadBotConfigurations();

    json body = json::object();
    for (auto &[id, config]: configs) {
        auto [start, end] = bots.equal_range(config.channel);
        auto it = std::find_if(start, end, [id = id] (const auto& pair) {
            return pair.second->getId() == id;
        });
        if (it == bots.end()) {
            bots.emplace(config.channel, std::make_shared<BotEngine>(config, irc, this->logger));
            body[std::to_string(config.botId)] = "created";
        } else {
            it->second->updateConfiguration(config);
            body[std::to_string(config.botId)] = "updated";
        }
    }
    return body.dump();
}
