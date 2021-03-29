//
// Created by imelker on 06.03.2021.
//

#include <memory>

#include <fmt/format.h>

#include "../common/Logger.h"
#include "../common/Clock.h"
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


const IRCConnectConfig &IRCWorker::getConnectConfig() const {
    return conConfig;
}

const IRCClientConfig &IRCWorker::getClientConfig() const {
    return ircConfig;
}

std::set<std::string> IRCWorker::getJoinedChannels() const {
    std::lock_guard lg(channelsMutex);
    return joinedChannels;
}

const decltype(IRCWorker::stats)& IRCWorker::getStats() const {
    return stats;
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

        client->registerHook("PRIVMSG", [this] (const IRCMessage& message) {
            this->messageHook(message);
        });

        if (!connect(conConfig, client.get(), logger.get()))
            break;

        auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.connects.updated.store(timestamp, std::memory_order_relaxed);
        stats.connects.attempts.fetch_add(1, std::memory_order_relaxed);

        logger->logInfo("IRCWorker[{}] connected to {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);
        listener->onConnected(this);

        if (!login(ircConfig, client.get(), logger.get()))
            break;

        logger->logInfo("IRCWorker[{}] logged in by {}:{}", fmt::ptr(this), ircConfig.user, ircConfig.nick);

        // rejoin to channels on reconnect
        int count = 0;
        {
            std::lock_guard lg(channelsMutex);
            for(auto &channel: joinedChannels) {
                if (sendIRC(fmt::format("JOIN #{}\n", channel))) {
                    logger->logTrace("IRCWorker[{}] {} joining to {}/{}", fmt::ptr(this), ircConfig.nick, channel, count++);
                }
            }
        }
        stats.channels.updated.store(count ? timestamp : 0, std::memory_order_relaxed);
        stats.channels.count.store(count, std::memory_order_relaxed);

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

bool IRCWorker::joinChannel(const std::string &channel, std::string &result) {
    {
        std::lock_guard lg(channelsMutex);
        if (joinedChannels.count(channel)) {
            logger->logInfo(R"(IRCWorker[{}] "{}" already joined to channel({}))", fmt::ptr(this), ircConfig.nick, channel);
            result = ircConfig.nick + " already joined";
            return true;
        }

        if (joinedChannels.size() >= static_cast<size_t>(ircConfig.channels_limit)) {
            logger->logError(R"(IRCWorker[{}] "{}" failed to join to channel({}). Limit reached)", fmt::ptr(this), ircConfig.nick, channel);
            result = "Channels limit reached";
            return false;
        }

        joinedChannels.emplace(channel);
    }

    if (sendIRC(fmt::format("JOIN #{}\n", channel))) {
        logger->logInfo("IRCWorker[{}] {} joining to channel({})", fmt::ptr(this), ircConfig.nick, channel);

        auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.channels.updated.store(timestamp, std::memory_order_relaxed);
        stats.channels.count.fetch_add(1, std::memory_order_relaxed);
        result = ircConfig.nick + "joined";
        return true;
    }
    return false;
}

void IRCWorker::leaveChannel(const std::string &channel) {
    {
        std::lock_guard lg(channelsMutex);
        if (joinedChannels.count(channel) == 0) {
            logger->logInfo(R"(IRCWorker[{}] "{}" already left from channel({}))", fmt::ptr(this), ircConfig.nick, channel);
            return;
        }

        joinedChannels.erase(channel);
    }

    if (sendIRC(fmt::format("PART #{}\n", channel))) {
        logger->logInfo("IRCWorker[{}] {} leaving from channel({})", fmt::ptr(this), ircConfig.nick, channel);
    }
}

bool IRCWorker::sendMessage(const std::string &channel, const std::string &text) {
    if (sendIRC(fmt::format("PRIVMSG #{} :{}\n", channel, text))) {
        logger->logInfo(R"(IRCWorker[{}] "{} send to "{}" message: "{}")", fmt::ptr(this), ircConfig.nick, channel, text);
        return true;
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

    if (client && client->connected() && client->sendIRC(message)) {
        auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.messages.out.updated.store(timestamp, std::memory_order_relaxed);
        stats.messages.out.count.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    return false;
}

void IRCWorker::messageHook(const IRCMessage& message) {
    auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();

    logger->logTrace("IRCWorker[{}] {}({}): {}", fmt::ptr(this), message.prefix.nickname,
                     message.parameters.at(0), message.parameters.back());
    listener->onMessage(this, std::move(message), timestamp);

    stats.messages.in.updated.store(timestamp, std::memory_order_relaxed);
    stats.messages.in.count.fetch_add(1, std::memory_order_relaxed);
}
