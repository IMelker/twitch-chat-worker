//
// Created by imelker on 02.04.2021.
//

#include <algorithm>

#include <nlohmann/json.hpp>

#include <so_5/environment.hpp>
#include <so_5/send_functions.hpp>

#include "../common/Logger.h"
#include "../DBController.h"
#include "BotsEnvironment.h"
#include "BotEngine.h"

using json = nlohmann::json;

BotsEnvironment::BotsEnvironment(const context_t &ctx,
                                 so_5::mbox_t publisher,
                                 DBController *db,
                                 std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), publisher(std::move(publisher)), db(db), logger(std::move(logger)) {
    ignoreUsers = db->loadUsersNicknames();
}

BotsEnvironment::~BotsEnvironment() {
    this->logger->logInfo("BotsEnvironment end of environment");
};

void BotsEnvironment::setMessageSender(so_5::mbox_t sender) {
    msgSender = std::move(sender);
}

void BotsEnvironment::setBotLogger(so_5::mbox_t blogger) {
    botLogger = std::move(blogger);
}

void BotsEnvironment::so_define_agent() {
    so_subscribe(publisher).event(&BotsEnvironment::evtChatMessage);
}

void BotsEnvironment::so_evt_start() {
    auto configs = db->loadBotConfigurations();
    so_5::introduce_child_coop(*this, [&] (so_5::coop_t &coop) {
        for (auto &[id, config]: configs) {
            auto it = botBoxes.find(config.channel);
            if (it == botBoxes.end()) {
                it = botBoxes.emplace(config.channel, so_environment().create_mbox(config.channel)).first;
            }
            auto *bot = coop.make_agent<BotEngine>(it->second, msgSender, botLogger, config, this->logger);

            botsById.emplace(config.botId, bot);
        }
    });
    this->logger->logInfo("BotsEnvironment {} bot created", configs.size());

    for(auto &[id, bot]: botsById) {
        bot->start();
    }
}

void BotsEnvironment::so_evt_finish() {
    for(auto &[id, bot]: botsById) {
        bot->stop();
    }
}

void BotsEnvironment::evtChatMessage(mhood_t<Chat::Message> msg) {
    if (!msg->valid || std::find(ignoreUsers.begin(), ignoreUsers.end(), msg->user) != ignoreUsers.end())
        return;

    auto it = botBoxes.find(msg->channel);
    if (it != botBoxes.end()) {
        so_5::send(it->second, msg.make_holder());
    }
}

std::tuple<int, std::string> BotsEnvironment::processHttpRequest(std::string_view path, const std::string &request,
                                                                 std::string &error) {
    if (path == "reloadconfiguration")
        return {200, handleReloadConfiguration(request, error)};

    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}

std::string BotsEnvironment::handleReloadConfiguration(const std::string &, std::string &error) {
    auto configs = db->loadBotConfigurations();
    if (configs.empty()) {
        error = "Failed to load configurations";
        return "";
    }

    json body = json::object();

    /*
    for (auto &[id, config]: configs) {
        auto it = botsById.find(id);
        if (it == botsById.end()) {
            {
                auto bot = addBot(config);
                bot->setSender(irc);
                bot->setBotLogger(botLogger);
                bot->start();
            }

            body[std::to_string(config.botId)] = "created";
        } else {
            it->second->stop();
            it->second->setConfig(config);
            it->second->start();

            body[std::to_string(config.botId)] = "updated";
        }
    }*/

    auto result = body.dump();
    this->logger->logInfo("BotsEnvironment bots configuration reloaded: {}", result);
    return result;
}
