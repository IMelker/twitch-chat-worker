//
// Created by l2pic on 25.04.2021.
//

#include <absl/strings/str_join.h>

#include "Clock.h"
#include "IRCMessage.h"
#include "IRCSessionListener.h"
#include "IRCSession.h"

#define PING_PONG_TIMEOUT_MS 10500

IRCSession::IRCSession(const IRCConnectionConfig &conConfig,
                       const IRCClientConfig &cliConfig,
                       unsigned int id,
                       IRCSessionListener *listener,
                       IRCClient *parent,
                       Logger *logger)
    : conConfig(conConfig), cliConfig(cliConfig), id(id), listener(listener), logger(logger) {
    ctx.callback = this;
    ctx.parent = parent;

    create();

    loggerTag = fmt::format("IRCSession[{}/{}/{}]", fmt::ptr(this),cliConfig.nick, id);
    logger->logInfo("{} Session init on IRCClient({}) for {}", loggerTag, fmt::ptr(parent), cliConfig.nick);

    auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
    lastPingTime = now;
    lastPongTime = now;
}

IRCSession::~IRCSession() {
    destroy();

    logger->logTrace("{} Session destroyed", loggerTag);
}

void IRCSession::create() {
    session = irc_create_session(this);

    irc_set_ctx(session, &ctx);

    irc_option_set(session, LIBIRC_OPTION_DEBUG);
    irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);
}

void IRCSession::destroy() {
    if (session) {
        irc_destroy_session(session);
        session = nullptr;
    }
}

bool IRCSession::connect() {
    // IRCClient thread
    if (irc_connect(session,
                    conConfig.host.c_str(), conConfig.port,
                    cliConfig.password.c_str(), cliConfig.nick.c_str(), cliConfig.user.c_str(), "IRC Client")) {
        ++statsFromSo5Thread.connects.failed;
        logger->logError("{} Could not connect: {}", loggerTag, irc_strerror(irc_errno(session)));
        return false;
    }

    auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
    lastPingTime = now;
    lastPongTime = now;
    logged.store(false, std::memory_order_relaxed);

    ++statsFromSo5Thread.connects.success;
    return true;
}

bool IRCSession::connected() {
    // IRCSelector && IRCClient thread
    return irc_is_connected(session);
}

bool IRCSession::loggedIn() {
    return logged.load(std::memory_order_relaxed);
}

void IRCSession::disconnect() {
    // IRCClient && IRCSelector thread
    irc_disconnect(session);
}

unsigned int IRCSession::getId() const {
    return id;
}

void IRCSession::setPingTimer(so_5::timer_id_t timer) {
    pingTimer = std::move(timer);
}

bool IRCSession::sendQuit(const std::string &reason) {
    logger->logTrace("{} Send QUIT: {}", loggerTag, reason);
    return internalIrcSend(irc_cmd_quit, reason.empty() ? nullptr : reason.c_str());
}

bool IRCSession::sendJoin(const std::string &channel) {
    logger->logTrace("{} Send JOIN: {}", loggerTag, channel);
    ++statsFromSo5Thread.commands.out.join;
    return internalIrcSend(irc_cmd_join, channel.c_str(), nullptr);
}

bool IRCSession::sendJoin(const std::string &channel, const std::string &key) {
    logger->logTrace("{} Send JOIN: {}:{}", loggerTag, channel, key);
    ++statsFromSo5Thread.commands.out.join;
    return internalIrcSend(irc_cmd_join, channel.c_str(), key.c_str());
}

bool IRCSession::sendPart(const std::string &channel) {
    logger->logTrace("{} Send PART: {}", loggerTag, channel);
    ++statsFromSo5Thread.commands.out.part;
    return internalIrcSend(irc_cmd_part, channel.c_str());
}

bool IRCSession::sendTopic(const std::string &channel, const std::string &topic) {
    logger->logTrace("{} Send TOPIC to {} as {}", loggerTag, channel, topic);
    return internalIrcSend(irc_cmd_topic, channel.c_str(), topic.c_str());
}

bool IRCSession::sendNames(const std::string &channel) {
    logger->logTrace("{} Send NAMES to {}", loggerTag, channel);
    return internalIrcSend(irc_cmd_names, channel.c_str());
}

bool IRCSession::sendList(const std::string &channel) {
    logger->logTrace("{} Send LIST to {}", loggerTag, channel);
    return internalIrcSend(irc_cmd_list, channel.empty() ? nullptr : channel.c_str());
}

bool IRCSession::sendInvite(const std::string &channel, const std::string &nick) {
    logger->logTrace("{} Send INVITE to {} on ", loggerTag, nick, channel);
    return internalIrcSend(irc_cmd_invite, nick.c_str(), channel.c_str());
}

bool IRCSession::sendKick(const std::string &channel, const std::string &nick, const std::string &comment) {
    logger->logTrace("{} Send KICK: {} from {} for \"{}\"", loggerTag, nick, channel, comment);
    return internalIrcSend(irc_cmd_kick, nick.c_str(), channel.c_str(), comment.empty() ? nullptr : comment.c_str());
}

bool IRCSession::sendMessage(const std::string &channel, const std::string &text) {
    logger->logTrace("{} Send PRIMSG from {} to {} : \"{}\"",
                     loggerTag, cliConfig.nick, channel, text);
    ++statsFromSo5Thread.commands.out.privmsg;
    return internalIrcSend(irc_cmd_msg, channel.c_str(), text.c_str());
}

bool IRCSession::sendNotice(const std::string &channel, const std::string &text) {
    logger->logTrace("{} Send NOTICE from {} to {} : \"{}\"",
                     loggerTag, cliConfig.nick, channel, text);
    return internalIrcSend(irc_cmd_notice, channel.c_str(), text.c_str());
}

bool IRCSession::sendCtcpRequest(const std::string &nick, const std::string &reply) {
    return internalIrcSend(irc_cmd_ctcp_request, nick.c_str(), reply.c_str());
}

bool IRCSession::sendCtcpReply(const std::string &nick, const std::string &reply) {
    return internalIrcSend(irc_cmd_ctcp_reply, nick.c_str(), reply.c_str());
}

bool IRCSession::sendMe(const std::string &channel, const std::string &text) {
    logger->logTrace("{} Send ME to {} : \"{}\"", loggerTag, channel, text);
    return internalIrcSend(irc_cmd_me, channel.c_str(), text.c_str());
}

bool IRCSession::sendChannelMode(const std::string &channel, const std::string &mode) {
    logger->logTrace("{} Send CHANNEL_MODE to {} as {}", loggerTag, channel, mode);
    return internalIrcSend(irc_cmd_channel_mode, channel.c_str(), mode.c_str());
}

bool IRCSession::sendUserMode(const std::string &mode) {
    logger->logTrace("{} Send USER_MODE as {}", loggerTag, mode);
    return internalIrcSend(irc_cmd_user_mode, mode.c_str());
}

bool IRCSession::sendNick(const std::string &newnick) {
    logger->logTrace("{} Send NICK as {}", loggerTag, newnick);
    return internalIrcSend(irc_cmd_nick, newnick.c_str());
}

bool IRCSession::sendWhois(const std::string &nick) {
    logger->logTrace("{} Send WHOIS for {}", loggerTag, nick);
    return internalIrcSend(irc_cmd_nick, nick.c_str());
}

bool IRCSession::sendPing(const std::string &host) {
    sendStats(statsFromSo5Thread);

    if (lastPingTime - lastPongTime >= PING_PONG_TIMEOUT_MS) {
        //logger->logError("{} PING/PONG timeout: {}", loggerTag, lastPingTime - lastPongTime);
        onDisconnected("TIMEOUT", fmt::format("PING/PONG timeout: {}", lastPingTime - lastPongTime));
        return false;
    }

    if (internalIrcSend(irc_cmd_ping, host.c_str())) {
        lastPingTime = CurrentTime<std::chrono::system_clock>::milliseconds();
        logger->logTrace("{} Send PING with host: {}", loggerTag, host);
        return true;
    }

    return false;
}

bool IRCSession::sendRaw(const std::string &raw) {
    logger->logTrace("{} Send RAW : \"{}\"", loggerTag, raw);
    return internalIrcSend(irc_send_raw, raw.c_str());
}

void IRCSession::onLog(const char *msg, int len) {
    // IRCSelector thread
    logger->logTrace("{} {}", loggerTag, std::string_view(msg, len - 1/*cut trailing next line*/));
}

void IRCSession::onConnected(std::string_view, std::string_view host) {
    // IRCSelector thread
    logger->logTrace("{} Connected to server: {}", loggerTag, host);
}

void IRCSession::onDisconnected(std::string_view, std::string_view reason) {
    // IRCSelector thread
    logger->logError("{} Disconnected from server: {}", loggerTag, reason);

    // remove current ping timer
    pingTimer = {};
    // clear internal session data
    disconnect();

    listener->onDisconnected(this, reason);
}

void IRCSession::onSendDataLen(int len) {
    // IRCSelector thread
    statsFromSelectorThread.commands.out.bytes += len;
}

void IRCSession::onRecvDataLen(int len) {
    // IRCSelector thread
    statsFromSelectorThread.commands.in.bytes += len;
}

void IRCSession::onLoggedIn(std::string_view /*event*/,
                            std::string_view origin,
                            const std::vector<std::string_view> &/*params*/) {
    // IRCSelector thread
    logger->logTrace("{} Successfully logged in: {}", loggerTag, origin);
    ++statsFromSelectorThread.connects.loggedin;

    logged.store(true, std::memory_order_relaxed);
    listener->onLoggedIn(this);
}

void IRCSession::onNick(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onQuit(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onJoin(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));

    if (!params.empty()) {
        listener->onJoined(this, params.front().substr(1/*cut #*/));
    }

    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onPart(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onMode(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onUmode(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onTopic(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onKick(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onChannel(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("{} Event {} received from {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;

    if (params.empty()) // invalid channel message
        return;

    listener->onMessage(IRCMessage{params.front()[0] == '#' ? params.front().substr(1) : params.front(),
                                   origin,
                                   params.back()});
}

void IRCSession::onPrivmsg(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onNotice(std::string_view event,
                          std::string_view origin,
                          const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onChannelNotice(std::string_view event,
                                 std::string_view origin,
                                 const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onInvite(std::string_view event,
                          std::string_view origin,
                          const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onCtcpReq(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onCtcpRep(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onCtcpAction(std::string_view event,
                              std::string_view origin,
                              const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;

    if (params.empty()) // invalid message
        return;

    listener->onMessage(IRCMessage{params.front()[0] == '#' ? params.front().substr(1) : params.front(),
                                   origin,
                                   params.back()});
}

void IRCSession::onPong(std::string_view event, std::string_view host) {
    // IRCSelector thread
    lastPongTime = CurrentTime<std::chrono::system_clock>::milliseconds();
    auto rtt = lastPongTime - lastPingTime;
    logger->logTrace("{} Event {} received on {}. RTT: {}", loggerTag, event, host, rtt);
    statsFromSelectorThread.commands.ping_pong.rtt = rtt;
    ++statsFromSelectorThread.commands.in.count;

    sendStats(statsFromSelectorThread);
}

void IRCSession::onUnknown(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onNumeric(unsigned int event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("{} Event {} received on {}, {{ {} }}", loggerTag, event, origin, absl::StrJoin(params, ", "));
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onDccChatReq(std::string_view nick, std::string_view addr, unsigned int dccid) {
    // IRCSelector thread
    //internalIrcRecv("DCC_CHAT", nick);
    logger->logWarn("{} Event DCC_CHAT received from {} on {}(dccid={})", loggerTag, nick, addr, dccid);
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::onDccSendReq(std::string_view nick,
                              std::string_view addr,
                              std::string_view /*filename*/,
                              unsigned long /*size*/,
                              unsigned int dccid) {
    // IRCSelector thread
    logger->logWarn("{} Event DCC_SEND received from {} on {}(dccid={})", loggerTag, nick, addr, dccid);
    ++statsFromSelectorThread.commands.in.count;
}

void IRCSession::sendStats(IRCStatistic &stats) {
    IRCStatistic temp{};
    std::swap(temp, stats);
    this->listener->onStatistics(this, std::move(temp));
}
