//
// Created by l2pic on 25.04.2021.
//

#include <cassert>
#include <utility>

#include <so_5/send_functions.hpp>

#include "Logger.h"

#include "IRCSelectorPool.h"
#include "IRCClient.h"
#include "IRCSession.h"

#define MIN_SESS_COUNT 1

IRCClient::IRCClient(const context_t &ctx,
                     so_5::mbox_t parent,
                     so_5::mbox_t processor,
                     IRCConnectionConfig conConfig,
                     IRCClientConfig cliConfig,
                     IRCSelectorPool *pool,
                     std::shared_ptr<Logger> logger,
                     std::shared_ptr<DBController> db)
    : so_5::agent_t(ctx),
      parent(std::move(parent)),
      processor(std::move(processor)),
      conConfig(std::move(conConfig)),
      cliConfig(std::move(cliConfig)),
      channels(sessions, this->cliConfig, logger, std::move(db)),
      logger(logger),
      pool(pool) {
    assert(pool);
    logger->logTrace("IRCClient[{}] init client for {}", fmt::ptr(this) , this->cliConfig.nick);
}

IRCClient::~IRCClient() {
    logger->logTrace("IRCClient[{}] end of client", fmt::ptr(this));
}

void IRCClient::addNewSession() {
    auto session = std::make_shared<IRCSession>(conConfig, cliConfig, this, this, logger.get());

    sessions.push_back(session);
    pool->addSession(session);

    logger->logTrace("IRCClient[{}] added new IRC session {}/{}",
                     fmt::ptr(this), sessions.size(), cliConfig.session_count);

    so_5::send<Connect>(so_direct_mbox(), session.get(), 0);
}

const std::string &IRCClient::nickname() const {
    return cliConfig.nick;
}

const IRCStatistic &IRCClient::statistic() {
    stats.clear();
    for(auto &session: sessions) {
        stats += session->statistic();
    }
    return IRCStatisticProvider::statistic();
}

void IRCClient::so_define_agent() {
    so_subscribe_self().event(&IRCClient::evtShutdown);
    so_subscribe_self().event(&IRCClient::evtConnect);

    so_subscribe_self().event(&IRCClient::evtReload);

    so_subscribe_self().event(&IRCClient::evtJoinChannels);
    so_subscribe_self().event(&IRCClient::evtJoinChannel, so_5::thread_safe);
    so_subscribe_self().event(&IRCClient::evtLeaveChannel, so_5::thread_safe);
    so_subscribe_self().event(&IRCClient::evtSendMessage, so_5::thread_safe);
    so_subscribe_self().event(&IRCClient::evtSendIRC, so_5::thread_safe);
    so_subscribe_self().event(&IRCClient::evtSendPING, so_5::thread_safe);
}

void IRCClient::so_evt_start() {
    channels.load();

    int count = std::max(MIN_SESS_COUNT, cliConfig.session_count);
    for (int i = 0; i < count; ++i) {
        addNewSession();
    }

    using std::chrono::seconds;
    pingTimer = so_5::send_periodic<Irc::SendPING>(so_direct_mbox(), seconds(10), seconds(10), "tmi.twitch.tv");
}

void IRCClient::so_evt_finish() {
    for (auto &session: sessions) {
        pool->removeSession(session);

        session->disconnect();
    }
}

void IRCClient::evtShutdown(so_5::mhood_t<Irc::Shutdown>) {
    so_deregister_agent_coop_normally();
}

void IRCClient::evtConnect(so_5::mhood_t<Connect> evt) {
    if (evt->session->connect()) {
        logger->logInfo("IRCClient[{}] IRCSession({}) Successfully connected to {} with username: {}, password: {}",
                        fmt::ptr(this), fmt::ptr(evt->session), conConfig.host, cliConfig.nick, cliConfig.password);
        return;
    }

    ++evt->attempt;

    if (evt->attempt == conConfig.connect_attemps_limit) {
        logger->logWarn("IRCClient[{}] IRCSession({}) Reconnect limit reached, restart attempts",
                        fmt::ptr(this), fmt::ptr(evt->session));
        evt->attempt = 0;
    }

    logger->logWarn("IRCClient[{}] IRCSession({}) Failed to connect. Reconnection after {} seconds",
                    fmt::ptr(this), fmt::ptr(evt->session), 2 * evt->attempt);

    so_5::send_delayed(so_direct_mbox(), std::chrono::seconds(2 * evt->attempt), evt);
}

void IRCClient::evtReload(so_5::mhood_t<Reload> evt) {
    cliConfig = evt->config;
    for(auto &session: sessions) {
        session->disconnect();
        so_5::send<Connect>(so_direct_mbox(), session.get(), 0);
    }
}

void IRCClient::evtJoinChannels(so_5::mhood_t<Irc::JoinChannels> evt) {
    for (auto &name : evt->channels)
        joinToChannel(name, getNextSessionRoundRobin());
}

void IRCClient::evtJoinChannel(so_5::mhood_t<Irc::JoinChannel> evt) {
    joinToChannel(evt->channel, getNextSessionRoundRobin());
}

void IRCClient::evtLeaveChannel(so_5::mhood_t<Irc::LeaveChannel> evt) {
    leaveFromChannel(evt->channel);
}

void IRCClient::evtSendMessage(so_5::mhood_t<Irc::SendMessage> message) {
    assert(message->account == cliConfig.nick);

    if (sendMessage(message->channel, message->text)) {
        logger->logInfo(R"(IRCClient[{}] {} send to "{}" message: "{}")",
                        fmt::ptr(this), cliConfig.nick, message->channel, message->text);
    }
}

void IRCClient::evtSendIRC(so_5::mhood_t<Irc::SendIRC> irc) {
    if (sendRaw(irc->message)) {
        logger->logInfo(R"(IRCClient[{}] {} send IRC "{}")", fmt::ptr(this), cliConfig.nick, irc->message);
    }
}

void IRCClient::evtSendPING(so_5::mhood_t<Irc::SendPING> ping) {
    sendPing(ping->host);
}

bool IRCClient::sendQuit(const std::string &reason) {
    for(auto &session: sessions) {
        session->sendQuit(reason);
    }
    return true;
}

bool IRCClient::sendJoin(const std::string &channel) {
    return getNextSessionRoundRobin()->sendJoin(channel);
}

bool IRCClient::sendJoin(const std::string &channel, const std::string &key) {
    return getNextSessionRoundRobin()->sendJoin(channel, key);
}

bool IRCClient::sendPart(const std::string &channel) {
    for (const auto& session: sessions) {
        session->sendPart(channel);
    }
    return true;
}

bool IRCClient::sendTopic(const std::string &channel, const std::string &topic) {
    return getNextSessionRoundRobin()->sendTopic(channel, topic);
}

bool IRCClient::sendNames(const std::string &channel) {
    return getNextSessionRoundRobin()->sendNames(channel);
}

bool IRCClient::sendList(const std::string &channel) {
    return getNextSessionRoundRobin()->sendList(channel);
}

bool IRCClient::sendInvite(const std::string &channel, const std::string &nick) {
    return getNextSessionRoundRobin()->sendInvite(channel, nick);
}

bool IRCClient::sendKick(const std::string &channel, const std::string &nick, const std::string &comment) {
    return getNextSessionRoundRobin()->sendKick(channel, nick, comment);
}

bool IRCClient::sendMessage(const std::string &channel, const std::string &text) {
    return getNextSessionRoundRobin()->sendMessage(channel, text);
}

bool IRCClient::sendNotice(const std::string &channel, const std::string &text) {
    return getNextSessionRoundRobin()->sendNotice(channel, text);
}

bool IRCClient::sendMe(const std::string &channel, const std::string &text) {
    return getNextSessionRoundRobin()->sendMe(channel, text);
}

bool IRCClient::sendChannelMode(const std::string &channel, const std::string &mode) {
    return getNextSessionRoundRobin()->sendChannelMode(channel, mode);
}

bool IRCClient::sendCtcpRequest(const std::string &nick, const std::string &reply) {
    return getNextSessionRoundRobin()->sendCtcpRequest(nick, reply);
}

bool IRCClient::sendCtcpReply(const std::string &nick, const std::string &reply) {
    return getNextSessionRoundRobin()->sendCtcpReply(nick, reply);
}

bool IRCClient::sendUserMode(const std::string &mode) {
    return getNextSessionRoundRobin()->sendUserMode(mode);
}

bool IRCClient::sendNick(const std::string &newnick) {
    return getNextSessionRoundRobin()->sendNick(newnick);
}

bool IRCClient::sendWhois(const std::string &nick) {
    return getNextSessionRoundRobin()->sendWhois(nick);
}

bool IRCClient::sendPing(const std::string &host) {
    for(auto &session: sessions) {
        session->sendPing(host);
    }
    return true;
}

bool IRCClient::sendRaw(const std::string &raw) {
    return getNextSessionRoundRobin()->sendRaw(raw);
}

void IRCClient::joinToChannel(const std::string &name, IRCSession *session) {
    if (channels.inList(name))
        return;

    Channel channel{name};
    channel.attach(session);
    if (session->sendJoin(name)) {
        logger->logInfo("IRCClient[{}] IRCSession({}) {} joining to channel({})",
                        fmt::ptr(this), fmt::ptr(session), cliConfig.nick, name);
        channels.addChannel(std::move(channel));
    } else
        logger->logError("IRCClient[{}] IRCSession({}) {} failed to join to channel({})",
                         fmt::ptr(this), fmt::ptr(session), cliConfig.nick, name);
}

void IRCClient::leaveFromChannel(const std::string &name) {
    auto channel = channels.extractChannel(name);
    if (!channel)
        return;
    channel->getSession()->sendPart(name);
}

IRCSession *IRCClient::getNextSessionRoundRobin() {
    if (sessions.size() == 1)
        return sessions.front().get();

    unsigned int i = curSessionRoundRobin++;
    curSessionRoundRobin %= sessions.size();
    return sessions[i].get();
}

void IRCClient::onLoggedIn(IRCSession *session) {
    auto joinList = channels.attachToSession(session);
    for (auto &channel: joinList) {
        session->sendJoin(channel);
    }
    logger->logInfo("IRCClient[{}] IRCSession({}) logged in. Joining to {} channels",
                     fmt::ptr(this), fmt::ptr(session), joinList.size());
}

void IRCClient::onDisconnected(IRCSession *session) {
    channels.detachFromSession(session);

    logger->logInfo("IRCClient[{}] IRCSession({}) Disconnected. Trying to reconnect",
                    fmt::ptr(this), fmt::ptr(session));

    so_5::send<Connect>(so_direct_mbox(), session, 0);
}

void IRCClient::onMessage(IRCMessage &&message) {
    so_5::send<IRCMessage>(processor, std::move(message));
}
