//
// Created by l2pic on 03.06.2021.
//
#include <absl/strings/match.h>
#include <so_5/send_functions.hpp>
#include <nlohmann/json.hpp>

#include "Clock.h"

#include "BotMessageEventHandlerCommand.h"
#include "../BotEvents.h"
#include "../BotEngine.h"

using json = nlohmann::json;

BotMessageEventHandlerCommand::BotMessageEventHandlerCommand(BotEngine * bot, int id, const std::string &text, const std::string &additional)
  : BotMessageEventHandler(bot, id, HandlerType::Command), text(text) {
    json add = json::parse(additional, nullptr, false, true);
    valid = !add.is_discarded() && !text.empty();

    if (valid) {
        auto &route = add["route"];
        if (!route.is_null()) {
            auto &t = route["text"];
            if (t.is_string())
                command = t.get<std::string>();
            auto &u = route["user"];
            if (u.is_string())
                user = u.get<std::string>();
        }
        auto &params = add["params"];
        if (!params.is_null()) {
            auto &m = params["mention"];
            if (m.is_boolean())
                mention = m.get<bool>();
        }

        valid = !command.empty();
    }
}

BotMessageEventHandlerCommand::~BotMessageEventHandlerCommand() = default;

void BotMessageEventHandlerCommand::handleBotMessage(const BotMessageEvent &evt) {
    auto const& message = evt.getMessage();
    if (!valid || !absl::StartsWith(message->text, command))
        return;

    if (!user.empty() && message->user != user)
        return;

    std::string sendText;
    if (mention) {
        sendText.reserve(text.size() + message->user.size() + sizeof("@ "));
        sendText.append("@").append(message->user).append(" ");
        sendText.append(text);
    } else {
        sendText = text;
    }

    so_5::send<Chat::SendMessage>(this->bot->getMsgSender(),
                                  this->bot->getConfig().account,
                                  this->bot->getConfig().channel,
                                  sendText);

    so_5::send<Bot::LogMessage>(this->bot->getBotLogger(),
                                this->bot->getConfig().userId,
                                this->bot->getConfig().botId,
                                this->getId(),
                                message->uuid.first,
                                CurrentTime<std::chrono::system_clock>::milliseconds(),
                                std::move(sendText));
}
