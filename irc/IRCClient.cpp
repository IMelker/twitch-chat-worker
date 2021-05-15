//
// Created by l2pic on 25.04.2021.
//

#include <cassert>
#include <utility>

#include <so_5/send_functions.hpp>

#include "Logger.h"
#include "ThreadName.h"

#include "IRCSelectorPool.h"
#include "IRCClient.h"
#include "IRCSession.h"

#define MIN_SESS_COUNT 1
#define LOGIN_TIMEOUT 3000
#define PING_DELAY_MS 2000
#define PING_TIMER(time) std::chrono::milliseconds{time}, std::chrono::milliseconds{time}

IRCClient::IRCClient(const context_t &ctx,
                     so_5::mbox_t statsCollector,
                     so_5::mbox_t processor,
                     IRCConnectionConfig conConfig,
                     IRCClientConfig cliConfig,
                     IRCSelectorPool *pool,
                     std::shared_ptr<Logger> logger,
                     std::shared_ptr<DBController> db)
    : so_5::agent_t(ctx),
      statsCollector(std::move(statsCollector)),
      processor(std::move(processor)),
      conConfig(std::move(conConfig)),
      cliConfig(std::move(cliConfig)),
      channels(sessions, this->cliConfig, logger, std::move(db)),
      pool(pool),
      logger(std::move(logger)) {
    assert(pool);
    loggerTag = fmt::format("IRCClient[{}/{}]", fmt::ptr(this) , this->cliConfig.nick);
    this->logger->logTrace("{} Client init", loggerTag);
}

IRCClient::~IRCClient() {
    logger->logTrace("{} Client destruction", loggerTag);
}

void IRCClient::addNewSession() {
    auto session = std::make_shared<IRCSession>(conConfig, cliConfig, sessions.size(), this, this, logger.get());

    sessions.push_back(session);
    pool->addSession(session);

    logger->logTrace("{} added new IRC session {}/{}",
                     loggerTag, sessions.size(), cliConfig.session_count);

    so_5::send<Connect>(so_direct_mbox(), session.get(), 0);
}

const std::string &IRCClient::nickname() const {
    return cliConfig.nick;
}

void IRCClient::so_define_agent() {
    so_subscribe_self().event(&IRCClient::evtShutdown);
    so_subscribe_self().event(&IRCClient::evtReload);
    so_subscribe_self().event(&IRCClient::evtConnect);
    so_subscribe_self().event(&IRCClient::evtJoinChannel);
    so_subscribe_self().event(&IRCClient::evtLeaveChannel);
    so_subscribe_self().event(&IRCClient::evtSendMessage);
    so_subscribe_self().event(&IRCClient::evtSendIRC);
    so_subscribe_self().event(&IRCClient::evtSendPING);
    so_subscribe_self().event(&IRCClient::evtLoggedInCheck);
}

void IRCClient::so_evt_start() {
    set_thread_name("irc_client");

    channels.load();

    int count = std::max(MIN_SESS_COUNT, cliConfig.session_count);
    for (int i = 0; i < count; ++i) {
        addNewSession();
    }
}

void IRCClient::so_evt_finish() {
    for (auto &session: sessions) {
        pool->removeSession(session);

        session->disconnect();
    }
}

void IRCClient::evtShutdown(so_5::mhood_t<Shutdown>) {
    so_deregister_agent_coop_normally();
}

void IRCClient::evtConnect(so_5::mhood_t<Connect> evt) {
    if (evt->session->connect()) {
        logger->logInfo("{} IRCSession({}) Successfully connected to {} with username: {}, password: {}",
                        loggerTag, fmt::ptr(evt->session), conConfig.host, cliConfig.nick, cliConfig.password);
        so_5::send_delayed<LoggedInCheck>(so_direct_mbox(), std::chrono::milliseconds(LOGIN_TIMEOUT), evt->session);
        return;
    }

    ++evt->attempt;

    if (evt->attempt == conConfig.connect_attemps_limit) {
        logger->logWarn("{} IRCSession({}) Reconnect limit reached, restart attempts",
                        loggerTag, fmt::ptr(evt->session));
        evt->attempt = 0;
    }

    logger->logWarn("{} IRCSession({}) Failed to connect. Reconnection after {} seconds",
                    loggerTag, fmt::ptr(evt->session), 2 * evt->attempt);

    so_5::send_delayed(so_direct_mbox(), std::chrono::seconds(2 * evt->attempt), evt);
}

void IRCClient::evtReload(so_5::mhood_t<Reload> evt) {
    logger->logInfo("{} Reload of {}", loggerTag, cliConfig);
    cliConfig = evt->config;
    loggerTag = fmt::format("IRCClient[{}/{}]", fmt::ptr(this) , cliConfig.nick);
    logger->logInfo("{} Reload to {}", loggerTag, cliConfig);

    // clear current sessions
    for (auto &session: sessions) {
        pool->removeSession(session);
        session->setPingTimer({});
        session->disconnect();
    }
    sessions.clear();

    // reload channels list
    channels.load();

    // start as new sessions
    int count = std::max(MIN_SESS_COUNT, cliConfig.session_count);
    for (int i = 0; i < count; ++i) {
        addNewSession();
    }
}

void IRCClient::evtLoggedInCheck(so_5::mhood_t<LoggedInCheck> evt) {
    if (!evt->session->loggedIn()) {
        evt->session->onDisconnected("LOGIN_TIMEOUT", fmt::format("Login timeout: {}ms", LOGIN_TIMEOUT));
    }
}

void IRCClient::evtJoinChannel(so_5::mhood_t<JoinChannel> evt) {
    joinToChannel(evt->channel, getNextConnectedSessionRoundRobin());
}

void IRCClient::evtLeaveChannel(so_5::mhood_t<LeaveChannel> evt) {
    leaveFromChannel(evt->channel);
}

void IRCClient::evtSendMessage(so_5::mhood_t<SendMessage> message) {
    if (sendMessage(message->channel, message->text)) {
        logger->logInfo(R"({} Send to "{}" message: "{}")",
                        loggerTag, message->channel, message->text);
    } else {
        logger->logError(R"({} Failed to send to "{}" message: "{}")",
                         loggerTag, message->channel, message->text);
    }
}

void IRCClient::evtSendIRC(so_5::mhood_t<SendIRC> irc) {
    if (sendRaw(irc->message)) {
        logger->logInfo("{} Send IRC \"{}\"", loggerTag, irc->message);
    } else {
        logger->logError("{} Failed to send IRC \"{}\"", loggerTag, irc->message);
    }
}

void IRCClient::evtSendPING(so_5::mhood_t<SendPING> ping) {
    so_5::send<Irc::ClientChannelsMetrics>(statsCollector, cliConfig.nick, channels.dumpChannelsToSessionId());

    ping->session->sendPing(ping->host);
}

bool IRCClient::sendQuit(const std::string &reason) {
    for(auto &session: sessions) {
        session->sendQuit(reason);
    }
    return true;
}

bool IRCClient::sendJoin(const std::string &channel) {
    return getNextConnectedSessionRoundRobin()->sendJoin(channel);
}

bool IRCClient::sendJoin(const std::string &channel, const std::string &key) {
    return getNextConnectedSessionRoundRobin()->sendJoin(channel, key);
}

bool IRCClient::sendPart(const std::string &channel) {
    for (const auto& session: sessions) {
        session->sendPart(channel);
    }
    return true;
}

bool IRCClient::sendTopic(const std::string &channel, const std::string &topic) {
    return getNextConnectedSessionRoundRobin()->sendTopic(channel, topic);
}

bool IRCClient::sendNames(const std::string &channel) {
    return getNextConnectedSessionRoundRobin()->sendNames(channel);
}

bool IRCClient::sendList(const std::string &channel) {
    return getNextConnectedSessionRoundRobin()->sendList(channel);
}

bool IRCClient::sendInvite(const std::string &channel, const std::string &nick) {
    return getNextConnectedSessionRoundRobin()->sendInvite(channel, nick);
}

bool IRCClient::sendKick(const std::string &channel, const std::string &nick, const std::string &comment) {
    return getNextConnectedSessionRoundRobin()->sendKick(channel, nick, comment);
}

bool IRCClient::sendMessage(const std::string &channel, const std::string &text) {
    return getNextConnectedSessionRoundRobin()->sendMessage(channel, text);
}

bool IRCClient::sendNotice(const std::string &channel, const std::string &text) {
    return getNextConnectedSessionRoundRobin()->sendNotice(channel, text);
}

bool IRCClient::sendMe(const std::string &channel, const std::string &text) {
    return getNextConnectedSessionRoundRobin()->sendMe(channel, text);
}

bool IRCClient::sendChannelMode(const std::string &channel, const std::string &mode) {
    return getNextConnectedSessionRoundRobin()->sendChannelMode(channel, mode);
}

bool IRCClient::sendCtcpRequest(const std::string &nick, const std::string &reply) {
    return getNextConnectedSessionRoundRobin()->sendCtcpRequest(nick, reply);
}

bool IRCClient::sendCtcpReply(const std::string &nick, const std::string &reply) {
    return getNextConnectedSessionRoundRobin()->sendCtcpReply(nick, reply);
}

bool IRCClient::sendUserMode(const std::string &mode) {
    return getNextConnectedSessionRoundRobin()->sendUserMode(mode);
}

bool IRCClient::sendNick(const std::string &newnick) {
    return getNextConnectedSessionRoundRobin()->sendNick(newnick);
}

bool IRCClient::sendWhois(const std::string &nick) {
    return getNextConnectedSessionRoundRobin()->sendWhois(nick);
}

bool IRCClient::sendPing(const std::string &host) {
    for(auto &session: sessions) {
        session->sendPing(host);
    }
    return true;
}

bool IRCClient::sendRaw(const std::string &raw) {
    return getNextConnectedSessionRoundRobin()->sendRaw(raw);
}

void IRCClient::joinToChannel(const std::string &name, IRCSession *session) {
    if (channels.inList(name))
        return;

    Channel channel{name};
    channel.attach(session);
    if (session->sendJoin(name)) {
        logger->logInfo("{} IRCSession({}) Joining to channel({})",
                        loggerTag, fmt::ptr(session), name);
        channels.addChannel(std::move(channel));
    } else
        logger->logError("{} IRCSession({}) Failed to join to channel({})",
                         loggerTag, fmt::ptr(session), name);
}

void IRCClient::leaveFromChannel(const std::string &name) {
    auto channel = channels.extractChannel(name);
    if (!channel) {
        logger->logWarn("{} Channel({}) to leave not found", loggerTag, name);
        return;
    }

    logger->logInfo("{} Leaving from channel({})", loggerTag, name);
    channel->getSession()->sendPart(name);
}

IRCSession *IRCClient::getNextSessionRoundRobin() {
    if (sessions.size() == 1)
        return sessions.front().get();

    unsigned int i = curSessionRoundRobin++;
    curSessionRoundRobin %= sessions.size();
    return sessions[i].get();
}

IRCSession *IRCClient::getNextConnectedSessionRoundRobin() {
    IRCSession *session = nullptr;
    for (size_t i = 0; i < sessions.size(); ++i) {
        // even if no one connected we will return session
        session = getNextSessionRoundRobin();
        if (session->connected())
            break;
    }
    return session;
}

void IRCClient::onLoggedIn(IRCSession *session) {
    auto joinList = channels.attachToSession(session);
    for (auto &channel: joinList) {
        session->sendJoin(channel);
    }
    logger->logInfo("{} IRCSession({}) logged in. Joining to {} channels",
                    loggerTag, fmt::ptr(session), joinList.size());

    session->setPingTimer(
        so_5::send_periodic<SendPING>(so_direct_mbox(), PING_TIMER(PING_DELAY_MS), session, "tmi.twitch.tv")
    );
}

void IRCClient::onDisconnected(IRCSession *session) {
    channels.detachFromSession(session);

    logger->logInfo("{} IRCSession({}) Disconnected. Trying to reconnect",
                    loggerTag, fmt::ptr(session));

    so_5::send<Connect>(so_direct_mbox(), session, 0);
}

void IRCClient::onStatistics(IRCSession *session, IRCStatistic &&stats) {
    so_5::send<Irc::SessionMetrics>(statsCollector, session->getId(), cliConfig.nick, std::move(stats));
}

void IRCClient::onMessage(IRCMessage &&message) {
    so_5::send<IRCMessage>(processor, std::move(message));
}

