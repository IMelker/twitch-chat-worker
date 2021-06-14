//
// Created by imelker on 02.04.2021.
//

#include <so_5/send_functions.hpp>

#include "Logger.h"
#include "ThreadName.h"

#include "../irc/IRCController.h"

#include "events/BotMessageEvent.h"
#include "handlers/BotEventHandler.h"
#include "handlers/BotMessageEventHandlerCommand.h"
#include "handlers/BotMessageEventHandlerLua.h"

#include "BotEngine.h"

BotEngine::BotEngine(const context_t &ctx, so_5::mbox_t self, so_5::mbox_t msgSender, so_5::mbox_t botLogger,
                     BotConfiguration config, std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), self(std::move(self)), msgSender(std::move(msgSender)), botLogger(std::move(botLogger)),
    logger(std::move(logger)), config(std::move(config)) {

    loadHandlers();

    this->logger->logInfo("BotEngine bot(id={}) created on user(id={}) for channel: {}",
                           this->config.botId, this->config.userId, this->config.channel);
}

BotEngine::~BotEngine() {
    this->logger->logTrace("BotEngine bot(id={}) destroyed", this->config.botId);
};

void BotEngine::loadHandlers() {
    auto emplaceHandler = [this](const EventHandlerConfiguration &cfg) {
        auto type = static_cast<BotEventHandler::HandlerType>(cfg.getTypeId());
        switch (type) {
            case BotEventHandler::HandlerType::Lua:
                massageHandlers.emplace_back(new BotMessageEventHandlerLua(this, cfg.getId(), cfg.getText(), cfg.getAdditional()));
                break;
            case BotEventHandler::HandlerType::Command:
                massageHandlers.emplace_back(new BotMessageEventHandlerCommand(this, cfg.getId(), cfg.getText(), cfg.getAdditional()));
                break;
            case BotEventHandler::HandlerType::Unknown:
            default:
                break;
        }
    };

    massageHandlers.clear();
    for(const auto & handlerConfig : this->config.handlers) {
        emplaceHandler(handlerConfig);
    }
}

void BotEngine::so_define_agent() {
    so_subscribe(self).event(&BotEngine::evtBotMessage);
    so_subscribe_self().event(&BotEngine::evtShutdown);
    so_subscribe_self().event(&BotEngine::evtReload);
}

void BotEngine::so_evt_start() {
    set_thread_name("bot_engine");

    this->logger->logInfo("BotEngine bot(id={}) started", this->config.botId);
}

void BotEngine::so_evt_finish() {
    this->logger->logInfo("BotEngine bot(id={}) stoped", this->config.botId);
}

void BotEngine::evtShutdown(so_5::mhood_t<Bot::Shutdown>) {
    so_deregister_agent_coop_normally();
}

void BotEngine::evtReload(so_5::mhood_t<Bot::Reload> message) {
    this->logger->logInfo("BotEngine bot(id={}) config update", this->config.botId);
    this->config = std::move(message->config);

    loadHandlers();
}

const BotConfiguration &BotEngine::getConfig() const {
    return config;
}

const std::shared_ptr<Logger> &BotEngine::getLogger() const {
    return logger;
}

const so_5::mbox_t &BotEngine::getMsgSender() const {
    return msgSender;
}

const so_5::mbox_t &BotEngine::getBotLogger() const {
    return botLogger;
}

void BotEngine::evtBotMessage(so_5::mhood_t<BotMessageEvent> evt) {
    // ignore self messages to avoid looping
    if (evt->getMessage()->user == config.account)
        return;

    for (auto& handler: massageHandlers) {
        handler->handleBotMessage(*evt);
    }
}
