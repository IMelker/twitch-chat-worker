//
// Created by imelker on 06.03.2021.
//

#include <memory>
#include <utility>

#include <fmt/format.h>
#include <so_5/send_functions.hpp>

#include "common/Logger.h"
#include "common/Clock.h"

#include "IRCWorker.h"

#define AUTH_ATTEMPTS_LIMIT 20

IRCWorker::IRCWorker(const context_t &ctx,
                     so_5::mbox_t parent,
                     so_5::mbox_t cc,
                     so_5::mbox_t processor,
                     IRCConnectConfig conConfig,
                     IRCClientConfig ircConfig,
                     std::shared_ptr<Logger> logger)
    : so_5::agent_t(ctx),
      parent(std::move(parent)),
      channelController(std::move(cc)),
      processor(std::move(processor)),
      logger(std::move(logger)),
      conConfig(std::move(conConfig)),
      ircConfig(std::move(ircConfig)),
      active(true),
      reconnect(false) {

}

IRCWorker::~IRCWorker() = default;

void IRCWorker::so_define_agent() {
    so_subscribe_self().event(&IRCWorker::evtShutdown);
    so_subscribe_self().event(&IRCWorker::evtReload);
    so_subscribe_self().event(&IRCWorker::evtInitAutoJoin, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtJoinChannels);
    so_subscribe_self().event(&IRCWorker::evtJoinChannel, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtLeaveChannels);
    so_subscribe_self().event(&IRCWorker::evtLeaveChannel, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtSendMessage, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtSendIRC, so_5::thread_safe);
    so_subscribe_self().event(&IRCWorker::evtSendPING, so_5::thread_safe);
}

void IRCWorker::so_evt_start() {
    thread = std::thread(&IRCWorker::run, this);

    pingTimer = so_5::send_periodic<Irc::SendPING>(so_direct_mbox(),
                                              std::chrono::seconds(10),
                                              std::chrono::seconds(10),
                                              "tmi.twitch.tv");
}

void IRCWorker::so_evt_finish() {
    active.store(false, std::memory_order_relaxed);
    if (thread.joinable())
        thread.join();
}

void IRCWorker::evtShutdown(so_5::mhood_t<Irc::Shutdown> evt) {
    so_deregister_agent_coop_normally();
}

void IRCWorker::evtReload(so_5::mhood_t<Reload> evt) {
    this->ircConfig = evt->config;
    reconnect.store(true, std::memory_order_relaxed);
}

void IRCWorker::evtInitAutoJoin(so_5::mhood_t<Irc::InitAutoJoin>) {
    so_5::send<Irc::GetChannels>(channelController, this, getFreeChannelSpace());
}

void IRCWorker::evtJoinChannels(so_5::mhood_t<Irc::JoinChannels> evt) {
    {
        std::lock_guard lg(channelsMutex);
        for(auto &channel : evt->channels) {
            if (sendIRC(fmt::format("JOIN #{}\n", channel))) {
                logger->logInfo("IRCWorker[{}] {} joining to channel({})", fmt::ptr(this), ircConfig.nick, channel);
                joinedChannels.emplace(channel);
            } else
                logger->logError("IRCWorker[{}] {} failed to join to channel({})", fmt::ptr(this), ircConfig.nick, channel);
        }
    }

    auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
    stats.channels.updated.store(timestamp, std::memory_order_relaxed);
    stats.channels.count.fetch_add(1, std::memory_order_relaxed);
}

void IRCWorker::evtJoinChannel(so_5::mhood_t<Irc::JoinChannel> evt) {
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

void IRCWorker::evtLeaveChannels(so_5::mhood_t<Irc::LeaveAllChannels>) {
    std::lock_guard lg(channelsMutex);
    for (auto& channel : joinedChannels) {
        if (sendIRC(fmt::format("PART #{}\n", channel))) {
            logger->logInfo("IRCWorker[{}] {} leaving from channel({})",
                            fmt::ptr(this), ircConfig.nick, channel);
        }
    }
    joinedChannels.clear();
}

void IRCWorker::evtLeaveChannel(so_5::mhood_t<Irc::LeaveChannel> evt) {
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

void IRCWorker::evtSendMessage(so_5::mhood_t<Irc::SendMessage> message) {
    assert(message->account == ircConfig.nick);

    if (sendIRC(fmt::format("PRIVMSG #{} :{}\n", message->channel, message->text))) {
        logger->logInfo(R"(IRCWorker[{}] "{} send to "{}" message: "{}")",
                        fmt::ptr(this), ircConfig.nick, message->channel, message->text);
    }
}

void IRCWorker::evtSendIRC(so_5::mhood_t<Irc::SendIRC> irc) {
    if (sendIRC(irc->message)) {
        logger->logInfo(R"(IRCWorker[{}] "{} send IRC "{}")", fmt::ptr(this), ircConfig.nick, irc->message);
    }
}

void IRCWorker::evtSendPING(so_5::mhood_t<Irc::SendPING> ping) {
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

int IRCWorker::getFreeChannelSpace() const {
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
    while (!SysSignal::serviceTerminated() && active.load(std::memory_order_relaxed)) {
        client = std::make_unique<IRCClient>(this, logger);

        if (!connect(conConfig, client.get(), logger.get()))
            break;

        // TODO remake stats, показатель ничего не дает
        auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.connects.updated.store(timestamp, std::memory_order_relaxed);
        stats.connects.attempts.fetch_add(1, std::memory_order_relaxed);

        logger->logInfo("IRCWorker[{}] connected to {}:{}", fmt::ptr(this), conConfig.host, conConfig.port);
        so_5::send<Irc::WorkerConnected>(parent, this);

        if (!login(ircConfig, client.get(), logger.get()))
            break;

        logger->logInfo("IRCWorker[{}] logged in by {}:{}", fmt::ptr(this), ircConfig.user, ircConfig.nick);
        so_5::send<Irc::WorkerLoggedIn>(parent, this);

        while (!SysSignal::serviceTerminated() &&
               active.load(std::memory_order_relaxed) &&
               client->connected()) {
            client->receive();
            if (reconnect.load(std::memory_order_relaxed)) {
                reconnect.store(false, std::memory_order_relaxed);
                break;
            }
        }
        logger->logInfo("IRCWorker[{}] {} disconnected from {}:{}", fmt::ptr(this), ircConfig.nick, conConfig.host, conConfig.port);

        clearChannels();

        so_5::send<Irc::WorkerDisconnected>(parent, this);
    }

    if (client->connected()) {
        client->disconnect();
        logger->logInfo("IRCWorker[{}] {} fully stopped on {}:{}", fmt::ptr(this), ircConfig.nick, conConfig.host, conConfig.port);
    }
}

void IRCWorker::clearChannels() {
    auto timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
    {
        std::lock_guard lg(channelsMutex);
        joinedChannels.clear();
    }
    stats.channels.updated.store(timestamp, std::memory_order_relaxed);
    stats.channels.count.store(0, std::memory_order_relaxed);
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
