//
// Created by imelker on 06.03.2021.
//

#include <memory>
#include <utility>

#include <fmt/format.h>
#include <so_5/send_functions.hpp>

#include "../common/Logger.h"
#include "../common/Clock.h"
#include "IRCWorker.h"

#define AUTH_ATTEMPTS_LIMIT 20

IRCWorker::IRCWorker(const context_t &ctx,
                     so_5::mbox_t parent,
                     so_5::mbox_t processor,
                     IRCConnectConfig conConfig,
                     IRCClientConfig ircConfig,
                     std::shared_ptr<Logger> logger)
    : so_5::agent_t(ctx), logger(std::move(logger)), parent(std::move(parent)), processor(std::move(processor)),
      conConfig(std::move(conConfig)), ircConfig(std::move(ircConfig)) {

}

IRCWorker::~IRCWorker() = default;

void IRCWorker::so_define_agent() {
    so_subscribe_self().event(&IRCWorker::evtJoinChannel, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtLeaveChannel, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtSendMessage, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtSendIRC, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtSendPING, so_5::thread_safe);
}

void IRCWorker::so_evt_start() {
    thread = std::thread(&IRCWorker::run, this);

    pingTimer = so_5::send_periodic<SendPING>(so_direct_mbox(),
                                              std::chrono::seconds(10),
                                              std::chrono::seconds(10),
                                              "tmi.twitch.tv");
}

void IRCWorker::so_evt_finish() {
    if (thread.joinable())
        thread.join();
}

void IRCWorker::evtJoinChannel(so_5::mhood_t<JoinChannel> evt) {
    {
        std::lock_guard lg(channelsMutex);
        if (joinedChannels.count(evt->channel)) {
            logger->logInfo(R"(IRCWorker[{}] "{}" already joined to channel({}))",
                            fmt::ptr(this), ircConfig.nick, evt->channel);
        }

        joinedChannels.emplace(evt->channel);
    }

    if (sendIRC(fmt::format("JOIN #{}\n", evt->channel))) {
        logger->logInfo("IRCWorker[{}] {} joining to channel({})", fmt::ptr(this), ircConfig.nick, evt->channel);

        auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.channels.updated.store(timestamp, std::memory_order_relaxed);
        stats.channels.count.fetch_add(1, std::memory_order_relaxed);
    }
}

void IRCWorker::evtLeaveChannel(so_5::mhood_t<LeaveChannel> evt) {
    {
        std::lock_guard lg(channelsMutex);
        if (joinedChannels.count(evt->channel) == 0) {
            logger->logInfo(R"(IRCWorker[{}] "{}" already left from channel({}))",
                            fmt::ptr(this), ircConfig.nick, evt->channel);
            return;
        }

        joinedChannels.erase(evt->channel);
    }

    if (sendIRC(fmt::format("PART #{}\n", evt->channel))) {
        logger->logInfo("IRCWorker[{}] {} leaving from channel({})",
                        fmt::ptr(this), ircConfig.nick, evt->channel);
    }
}

void IRCWorker::evtSendMessage(so_5::mhood_t<SendMessage> message) {
    assert(message->account == ircConfig.nick);

    if (sendIRC(fmt::format("PRIVMSG #{} :{}\n", message->channel, message->text))) {
        logger->logInfo(R"(IRCWorker[{}] "{} send to "{}" message: "{}")",
                        fmt::ptr(this), ircConfig.nick, message->channel, message->text);
    }
}

void IRCWorker::evtSendIRC(so_5::mhood_t<SendIRC> irc) {
    if (sendIRC(irc->message)) {
        logger->logInfo(R"(IRCWorker[{}] "{} send IRC "{}")", fmt::ptr(this), ircConfig.nick, irc->message);
    }
}

void IRCWorker::evtSendPING(so_5::mhood_t<SendPING> ping) {
    if (sendIRC(fmt::format("PING :{}\n", ping->host))) {
        logger->logTrace(R"(IRCWorker[{}] "{} send ping to "{}")", fmt::ptr(this), ircConfig.nick, ping->host);
    }
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

int IRCWorker::freeChannelSpace() const {
    return ircConfig.channels_limit - static_cast<int>(joinedChannels.size());
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
        client = std::make_unique<IRCClient>(this, logger);

        if (!connect(conConfig, client.get(), logger.get()))
            break;

        auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.connects.updated.store(timestamp, std::memory_order_relaxed);
        stats.connects.attempts.fetch_add(1, std::memory_order_relaxed);

        logger->logInfo("IRCWorker[{}] connected to {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);
        so_5::send<WorkerConnected>(parent, this);

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

        so_5::send<WorkerLoggedIn>(parent, this);

        while (!SysSignal::serviceTerminated() && client->connected()) {
            client->receive();
        }

        logger->logInfo("IRCWorker[{}] disconnected from {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);
        so_5::send<WorkerDisconnected>(parent, this);
    }

    if (client->connected()) {
        client->disconnect();
        logger->logInfo("IRCWorker[{}] fully disconnected from {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);

        so_5::send<WorkerDisconnected>(parent, this);
    }
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

void IRCWorker::onMessage(const IRCMessage &message) {
    stats.messages.in.updated.store(message.timestamp, std::memory_order_relaxed);
    stats.messages.in.count.fetch_add(1, std::memory_order_relaxed);

    so_5::send<IRCMessage>(processor, message);
}

