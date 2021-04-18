//
// Created by imelker on 02.04.2021.
//

#include <algorithm>

#include <nlohmann/json.hpp>

#include <so_5/coop.hpp>
#include <so_5/environment.hpp>
#include <so_5/send_functions.hpp>

#include "../common/Logger.h"
#include "../DBController.h"
#include "BotConfiguration.h"
#include "BotsEnvironment.h"
#include "BotEngine.h"

#define resp(str) json{{"result", str}}.dump()

using json = nlohmann::json;

BotsEnvironment::BotsEnvironment(const context_t &ctx,
                                 so_5::mbox_t publisher,
                                 so_5::mbox_t http,
                                 DBController *db,
                                 std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), publisher(std::move(publisher)), http(std::move(http)), db(db), logger(std::move(logger)) {
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
    so_subscribe(publisher).event(&BotsEnvironment::evtChatMessage, so_5::thread_safe);
    so_subscribe(http).event(&BotsEnvironment::evtHttpAdd);
    so_subscribe(http).event(&BotsEnvironment::evtHttpReload);
    so_subscribe(http).event(&BotsEnvironment::evtHttpRemove);
    so_subscribe(http).event(&BotsEnvironment::evtHttpReloadAll);
}

void BotsEnvironment::so_evt_start() {
    auto configs = db->loadBotConfigurations();
    for (auto &[id, config]: configs)
        addBot(config);
    this->logger->logInfo("BotsEnvironment {} bot created", configs.size());
}

void BotsEnvironment::so_evt_finish() {

}

void BotsEnvironment::addBot(const BotConfiguration &config) {
    auto it = botBoxes.find(config.channel);
    if (it == botBoxes.end()) {
        it = botBoxes.emplace(config.channel, so_environment().create_mbox(config.channel)).first;
    }

    auto *bot = so_5::introduce_child_coop(*this, [&box = it->second, &config, this] (so_5::coop_t &coop) {
        return coop.make_agent<BotEngine>(box, msgSender, botLogger, config, logger);
    });
    botsById.emplace(config.botId, bot);
}

void BotsEnvironment::evtChatMessage(mhood_t<Chat::Message> msg) {
    if (!msg->valid || std::find(ignoreUsers.begin(), ignoreUsers.end(), msg->user) != ignoreUsers.end())
        return;

    auto it = botBoxes.find(msg->channel);
    if (it != botBoxes.end()) {
        so_5::send(it->second, msg.make_holder());
    }
}

void BotsEnvironment::evtHttpAdd(mhood_t<hreq::bot::add> add) {
    logger->logTrace("BotsEnvironment Add new bot");

    json req = json::parse(add->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return so_5::send<hreq::resp>(http, std::move(add->req), std::move(add->send), 501, resp("Failed to parse JSON"));

    const auto& botId = req["id"];
    if (!botId.is_number())
        return so_5::send<hreq::resp>(http, std::move(add->req), std::move(add->send), 400, resp("Wrong bot id"));

    int id = botId.get<int>();
    auto it = botsById.find(id);
    if (it != botsById.end())
        return so_5::send<hreq::resp>(http, std::move(add->req), std::move(add->send), 403, resp("Bot already added"));

    auto config = db->loadBotConfiguration(id);
    if (config.botId == 0)
        return so_5::send<hreq::resp>(http, std::move(add->req), std::move(add->send), 404, resp("Bot not found in DB"));

    addBot(config);

    json body = {{"id", config.botId}, {"result", "Bot added"}};
    so_5::send<hreq::resp>(http, std::move(add->req), std::move(add->send), 200, body.dump());
}

void BotsEnvironment::evtHttpRemove(mhood_t<hreq::bot::remove> rem) {
    json req = json::parse(rem->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return so_5::send<hreq::resp>(http, std::move(rem->req), std::move(rem->send), 501, resp("Failed to parse JSON"));

    const auto& botId = req["id"];
    if (!botId.is_number())
        return so_5::send<hreq::resp>(http, std::move(rem->req), std::move(rem->send), 400, resp("Wrong bot id"));

    int id = botId.get<int>();
    auto it = botsById.find(id);
    if (it == botsById.end())
        return so_5::send<hreq::resp>(http, std::move(rem->req), std::move(rem->send), 404, resp("Bot not found"));

    // TODO add remove mbox
    so_5::send<BotEngine::Shutdown>(it->second->so_direct_mbox());
    botsById.erase(it);
    logger->logTrace("BotsEnvironment Remove bot(id={})", id);

    json body = {{"botId", id}, {"result", "Bot removed"}};
    so_5::send<hreq::resp>(http, std::move(rem->req), std::move(rem->send), 200, body.dump());
}

void BotsEnvironment::evtHttpReload(mhood_t<hreq::bot::reload> rel) {
    json req = json::parse(rel->req.body(), nullptr, false, true);
    if (req.is_discarded())
        return so_5::send<hreq::resp>(http, std::move(rel->req), std::move(rel->send), 501, resp("Failed to parse JSON"));

    const auto& botId = req["id"];
    if (!botId.is_number())
        return so_5::send<hreq::resp>(http, std::move(rel->req), std::move(rel->send), 400, resp("Wrong bot id"));

    int id = botId.get<int>();
    auto it = botsById.find(id);
    if (it == botsById.end())
        return so_5::send<hreq::resp>(http, std::move(rel->req), std::move(rel->send), 404, resp("Bot not found"));

    auto config = db->loadBotConfiguration(id);
    if (config.botId == 0)
        return so_5::send<hreq::resp>(http, std::move(rel->req), std::move(rel->send), 404, resp("Bot not found in DB"));

    so_5::send<BotEngine::Reload>(it->second->so_direct_mbox(), std::move(config));
    logger->logTrace("BotsEnvironment Reload bot(id={})", id);

    json body = {{"botId", id}, {"result", "Bot reloaded"}};
    so_5::send<hreq::resp>(http, std::move(rel->req), std::move(rel->send), 200, body.dump());
}

void BotsEnvironment::evtHttpReloadAll(mhood_t<hreq::bot::reloadall> rel) {
    // TODO hanlde unregistered bots
    auto configs = db->loadBotConfigurations();
    for (auto &[id, config]: configs) {
        auto it = botsById.find(id);
        if (it == botsById.end()) {
            addBot(config);
        }
        else {
            so_5::send<BotEngine::Reload>(it->second->so_direct_mbox(), std::move(config));
        }
    }
    this->logger->logInfo("BotsEnvironment Configuration reloaded for {} bots", configs.size());

    json body = {{"result", "Reloaded"}};
    so_5::send<hreq::resp>(http, std::move(rel->req), std::move(rel->send), 200, body.dump());
}
