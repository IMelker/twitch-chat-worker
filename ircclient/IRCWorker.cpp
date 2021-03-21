//
// Created by imelker on 06.03.2021.
//

#include <memory>
#include <utility>

#include <fmt/format.h>

#include "../common/Logger.h"
#include "IRCWorker.h"

#define AUTH_ATTEMPTS_LIMIT 20

IRCWorker::IRCWorker(IRCConnectConfig conConfig, IRCClientConfig ircConfig, IRCWorkerListener *listener, std::shared_ptr<Logger> logger)
    : logger(std::move(logger)), listener(listener), conConfig(std::move(conConfig)), ircConfig(std::move(ircConfig)) {
    thread = std::thread(&IRCWorker::run, this);
}

IRCWorker::~IRCWorker() {
    if (thread.joinable())
        thread.join();
}

inline bool connect(const IRCConnectConfig &cfg, IRCClient *client, Logger *logger) {
    int attempt = 0;
    while (attempt < cfg.connect_attemps_limit) {
        std::this_thread::sleep_for (std::chrono::seconds(2 * attempt));

        if (client->connect((char *)cfg.host.c_str(), cfg.port))
            return true;

        logger->logError("Failed to connect IRC with hostname: {}:{}, attempt: {}", cfg.host, cfg.port, attempt);

        ++attempt;
    }
    return false;
}

inline bool login(const IRCClientConfig &cfg, IRCClient *client, Logger *logger) {
    int attempts = AUTH_ATTEMPTS_LIMIT;
    while(--attempts > 0 && client->connected()) {
        if (client->login(cfg.nick, cfg.user, cfg.password))
            return true;

        logger->logError("Failed to login IRC with nick: {}, user: {}, attempts left: {}\n", cfg.nick, cfg.user, attempts);

        std::this_thread::sleep_for(std::chrono::milliseconds ((1000 / cfg.auth_per_sec_limit) - 1));
    }
    return false;
}

void IRCWorker::run() {
    while (!SysSignal::serviceTerminated()) {
        client = std::make_unique<IRCClient>();

        client->registerHook("PRIVMSG", [this] (IRCMessage message) {
            this->messageHook(std::move(message));
        });

        if (!connect(conConfig, client.get(), logger.get()))
            break;

        logger->logInfo("IRCWorker[{}] connected to {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);
        listener->onConnected(this);

        if (!login(ircConfig, client.get(), logger.get()))
            break;

        logger->logInfo("IRCWorker[{}] logged in by {}:{}", fmt::ptr(this), ircConfig.user, ircConfig.nick);

        for(auto &channel: joinedChannels) {
            logger->logTrace("IRCWorker[{}] {} joining to {}", fmt::ptr(this), ircConfig.nick, channel);
            client->sendIRC(fmt::format("JOIN #{}\n", channel));
        }

        listener->onLogin(this);

        while (!SysSignal::serviceTerminated() && client->connected()) {
            client->receive();
        }

        logger->logInfo("IRCWorker[{}] disconnected from {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);
        listener->onDisconnected(this);
    }

    if (client->connected()) {
        client->disconnect();
        logger->logInfo("IRCWorker[{}] fully disconnected from {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);
        listener->onDisconnected(this);
    }
}


bool IRCWorker::joinChannel(const std::string &channel) {
    if (joinedChannels.count(channel)) {
        logger->logInfo(R"(IRCWorker[{}] "{}" already joined to channel({}))", fmt::ptr(this), ircConfig.nick, channel);
        return true;
    }

    if (joinedChannels.size() >= static_cast<size_t>(ircConfig.channels_limit)) {
        logger->logError(R"(IRCWorker[{}] "{}" failed to join to channel({}). Limit reached)", fmt::ptr(this), ircConfig.nick, channel);
        return false;
    }

    joinedChannels.emplace(channel);

    if (client && client->connected()) {
        logger->logInfo("IRCWorker[{}] {} joining to {}", fmt::ptr(this), ircConfig.nick, channel);
        client->sendIRC(fmt::format("JOIN #{}\n", channel));
    }
    return true;
}

void IRCWorker::leaveChannel(const std::string &channel) {
    if (joinedChannels.count(channel) == 0) {
        logger->logInfo(R"(IRCWorker[{}] "{}" already left from channel({}))", fmt::ptr(this), ircConfig.nick, channel);
        return;
    }

    joinedChannels.erase(channel);

    if (client && client->connected()) {
        logger->logTrace("IRCWorker[{}] {} leaving {}", fmt::ptr(this), ircConfig.nick, channel);
        client->sendIRC(fmt::format("PART #{}\n", channel));
    }
}

bool IRCWorker::sendMessage(const std::string &channel, const std::string &text) {
    if (client && client->connected()) {
        logger->logTrace(R"(IRCWorker[{}] "{} send to "{}" message "{}")", fmt::ptr(this), ircConfig.nick, channel, text);
        return client->sendIRC(fmt::format("PRIVMSG #{} {}\n", channel, text));
    }
    return false;
}

bool IRCWorker::sendIRC(const std::string& message) {
    // 20 per 30 seconds: Users sending commands or messages to channels in which
    // they do not have Moderator or Operator status

    // 100 per 30 seconds: Users sending commands or messages to channels in which
    // they have Moderator or Operator status

    // For Whispers, which are private chat message between two users:
    // 3 per second, up to 100 per minute(40 accounts per day)

    if (client && client->connected())
        client->sendIRC(message);
}

void IRCWorker::messageHook(IRCMessage message) {
    logger->logTrace("IRCWorker[{}] {}({}): {}", fmt::ptr(this), message.prefix.nickname,
                     message.parameters.at(0), message.parameters.back());
    listener->onMessage(std::move(message), this);
}
