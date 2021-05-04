//
// Created by l2pic on 25.04.2021.
//

#include <absl/strings/str_join.h>

#include "Clock.h"
#include "IRCMessage.h"
#include "IRCSessionListener.h"
#include "IRCSession.h"

#define PING_PONG_TIMEOUT 30000

IRCSession::IRCSession(const IRCConnectionConfig &conConfig, const IRCClientConfig &cliConfig,
                       IRCSessionListener *listener, IRCClient *parent, Logger* logger)
    : conConfig(conConfig), cliConfig(cliConfig), listener(listener), logger(logger) {
    session = irc_create_session(this);

    ctx.callback = this;
    ctx.parent = parent;

    irc_set_ctx(session, &ctx);

    irc_option_set(session, LIBIRC_OPTION_DEBUG);
    irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);

    logger->logInfo("IRCSession[{}] session init on IRCClient({}) for {}", fmt::ptr(this), fmt::ptr(parent), cliConfig.nick);

    auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
    lastPingTime = now;
    lastPongTime = now;
}

IRCSession::~IRCSession() {
    irc_destroy_session(session);
    logger->logTrace("IRCSession[{}] session destroyed", fmt::ptr(this));
}

bool IRCSession::connect() {
    // IRCClient thread
    if (irc_connect(session,
                    conConfig.host.c_str(), conConfig.port,
                    cliConfig.password.c_str(), cliConfig.nick.c_str(), cliConfig.user.c_str(), "IRC Client")) {
        stats.connectsFailedInc();
        logger->logError("IRCSession[{}] Could not connect: %s", fmt::ptr(this), irc_strerror(irc_errno(session)));
        return false;
    }
    stats.connectsSuccessInc();
    stats.connectsUpdatedSet(CurrentTime<std::chrono::system_clock>::milliseconds());
    return true;
}

bool IRCSession::connected() {
    // IRCSelector thread
    return irc_is_connected(session);
}

void IRCSession::disconnect() {
    // IRCClient && IRCSelector thread
    irc_disconnect(session);
}

bool IRCSession::sendQuit(const std::string &reason) {
    logger->logTrace("IRCSession[{}] Send QUIT: {}", fmt::ptr(this), reason);
    return internalIrcSend(irc_cmd_quit, reason.empty() ? nullptr : reason.c_str());
}

bool IRCSession::sendJoin(const std::string &channel) {
    logger->logTrace("IRCSession[{}] Send JOIN: {}", fmt::ptr(this), channel);
    stats.channelsCountInc();
    return internalIrcSend(irc_cmd_join, channel.c_str(), nullptr);
}

bool IRCSession::sendJoin(const std::string &channel, const std::string &key) {
    logger->logTrace("IRCSession[{}] Send JOIN: {}:{}", fmt::ptr(this), channel, key);
    stats.channelsCountInc();
    return internalIrcSend(irc_cmd_join, channel.c_str(), key.c_str());
}

bool IRCSession::sendPart(const std::string &channel) {
    logger->logTrace("IRCSession[{}] Send PART: {}", fmt::ptr(this), channel);
    stats.channelsCountDec();
    return internalIrcSend(irc_cmd_part, channel.c_str());
}

bool IRCSession::sendTopic(const std::string &channel, const std::string &topic) {
    logger->logTrace("IRCSession[{}] Send TOPIC to {} as {}", fmt::ptr(this), channel, topic);
    return internalIrcSend(irc_cmd_topic, channel.c_str(), topic.c_str());
}

bool IRCSession::sendNames(const std::string &channel) {
    logger->logTrace("IRCSession[{}] Send NAMES to {}", fmt::ptr(this), channel);
    return internalIrcSend(irc_cmd_names, channel.c_str());
}

bool IRCSession::sendList(const std::string &channel) {
    logger->logTrace("IRCSession[{}] Send LIST to {}", fmt::ptr(this), channel);
    return internalIrcSend(irc_cmd_list, channel.empty() ? nullptr : channel.c_str());
}

bool IRCSession::sendInvite(const std::string &channel, const std::string &nick) {
    logger->logTrace("IRCSession[{}] Send INVITE to {} on ", fmt::ptr(this), nick, channel);
    return internalIrcSend(irc_cmd_invite, nick.c_str(), channel.c_str());
}

bool IRCSession::sendKick(const std::string &channel, const std::string &nick, const std::string &comment) {
    logger->logTrace("IRCSession[{}] Send KICK: {} from {} for \"{}\"", fmt::ptr(this), nick, channel, comment);
    return internalIrcSend(irc_cmd_kick, nick.c_str(), channel.c_str(), comment.empty() ? nullptr : comment.c_str());
}

bool IRCSession::sendMessage(const std::string &channel, const std::string &text) {
    logger->logTrace("IRCSession[{}] Send PRIMSG from {} to {} : \"{}\"",
                     fmt::ptr(this), cliConfig.nick, channel, text);
    return internalIrcSend(irc_cmd_msg, channel.c_str(), text.c_str());
}

bool IRCSession::sendNotice(const std::string &channel, const std::string &text) {
    logger->logTrace("IRCSession[{}] Send NOTICE from {} to {} : \"{}\"",
                     fmt::ptr(this), cliConfig.nick, channel, text);
    return internalIrcSend(irc_cmd_notice, channel.c_str(), text.c_str());
}

bool IRCSession::sendCtcpRequest(const std::string &nick, const std::string &reply) {
    return internalIrcSend(irc_cmd_ctcp_request, nick.c_str(), reply.c_str());
}

bool IRCSession::sendCtcpReply(const std::string &nick, const std::string &reply) {
    return internalIrcSend(irc_cmd_ctcp_reply, nick.c_str(), reply.c_str());
}

bool IRCSession::sendMe(const std::string &channel, const std::string &text) {
    logger->logTrace("IRCSession[{}] Send ME to {} : \"{}\"", fmt::ptr(this), channel, text);
    return internalIrcSend(irc_cmd_me, channel.c_str(), text.c_str());
}

bool IRCSession::sendChannelMode(const std::string &channel, const std::string &mode) {
    logger->logTrace("IRCSession[{}] Send CHANNEL_MODE to {} as {}", fmt::ptr(this), channel, mode);
    return internalIrcSend(irc_cmd_channel_mode, channel.c_str(), mode.c_str());
}

bool IRCSession::sendUserMode(const std::string &mode) {
    logger->logTrace("IRCSession[{}] Send USER_MODE as {}", fmt::ptr(this), mode);
    return internalIrcSend(irc_cmd_user_mode, mode.c_str());
}

bool IRCSession::sendNick(const std::string &newnick) {
    logger->logTrace("IRCSession[{}] Send NICK as {}", fmt::ptr(this), newnick);
    return internalIrcSend(irc_cmd_nick, newnick.c_str());
}

bool IRCSession::sendWhois(const std::string &nick) {
    logger->logTrace("IRCSession[{}] Send WHOIS for {}", fmt::ptr(this), nick);
    return internalIrcSend(irc_cmd_nick, nick.c_str());
}

bool IRCSession::sendPing(const std::string &host) {
    logger->logTrace("IRCSession[{}] Send PING with host: {}", fmt::ptr(this), host);
    if (lastPingTime - lastPongTime >= PING_PONG_TIMEOUT) {
        onDisconnected("TIMEOUT", "PING/PONG timeout");
        return false;
    }
    lastPingTime = CurrentTime<std::chrono::system_clock>::milliseconds();
    return internalIrcSend(irc_cmd_ping, host.c_str());
}

bool IRCSession::sendRaw(const std::string &raw) {
    logger->logTrace("IRCSession[{}] Send RAW : \"{}\"", fmt::ptr(this), raw);
    return internalIrcSend(irc_send_raw, raw.c_str());
}

void IRCSession::onLog(const char *msg, int len) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] {}", fmt::ptr(this), std::string_view(msg, len - 1/*cut trailing next line*/));
}

void IRCSession::onConnected(std::string_view, std::string_view host) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Connected to server: {}", fmt::ptr(this), host);
}

void IRCSession::onDisconnected(std::string_view, std::string_view reason) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Disconnected from server: {}", fmt::ptr(this), reason);
    stats.connectsDisconnectedInc();

    // clear internal session data
    disconnect();

    listener->onDisconnected(this);
}

void IRCSession::onSendDataLen(int len) {
    // IRCSelector thread
    stats.commandsOutBytesAdd(len);
}

void IRCSession::onRecvDataLen(int len) {
    // IRCSelector thread
    stats.commandsInBytesAdd(len);
}

void IRCSession::onLoggedIn(std::string_view event,
                            std::string_view origin,
                            const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Successfully logged in: {}", fmt::ptr(this), origin);
    stats.connectsLoggedInInc();

    listener->onLoggedIn(this);
}

/*inline std::ostream& operator<<(std::ostream& os, const std::vector<std::string_view> &params) {
    os << "params: {";
    for(const auto &param: params) {
        os << param << ", ";
    }
    return os << "}";
}*/

void IRCSession::onNick(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onQuit(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onJoin(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onPart(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onMode(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onUmode(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onTopic(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onKick(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onChannel(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Event {} received from {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();

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
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onNotice(std::string_view event,
                          std::string_view origin,
                          const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onChannelNotice(std::string_view event,
                                 std::string_view origin,
                                 const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onInvite(std::string_view event,
                          std::string_view origin,
                          const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onCtcpReq(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onCtcpRep(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onCtcpAction(std::string_view event,
                              std::string_view origin,
                              const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();

    if (params.empty()) // invalid message
        return;

    listener->onMessage(IRCMessage{params.front()[0] == '#' ? params.front().substr(1) : params.front(),
                                   origin,
                                   params.back()});
}

void IRCSession::onPong(std::string_view event, std::string_view host) {
    lastPongTime = CurrentTime<std::chrono::system_clock>::milliseconds();
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Event {} received on {}", fmt::ptr(this), event, host);
    stats.commandsInCountInc();
}


void IRCSession::onUnknown(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onNumeric(unsigned int event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCSelector thread
    logger->logTrace("IRCSession[{}] Event {} received on {}, {{ {} }}", fmt::ptr(this), event, origin, absl::StrJoin(params, ", "));
    stats.commandsInCountInc();
}

void IRCSession::onDccChatReq(std::string_view nick, std::string_view addr, unsigned int dccid) {
    // IRCSelector thread
    //internalIrcRecv("DCC_CHAT", nick);
    logger->logWarn("IRCSession[{}] Event DCC_CHAT received from {} on {}(dccid={})", fmt::ptr(this), nick, addr, dccid);
    stats.commandsInCountInc();
}

void IRCSession::onDccSendReq(std::string_view nick,
                              std::string_view addr,
                              std::string_view filename,
                              unsigned long size,
                              unsigned int dccid) {
    // IRCSelector thread
    logger->logWarn("IRCSession[{}] Event DCC_SEND received from {} on {}(dccid={})", fmt::ptr(this), nick, addr, dccid);
    stats.commandsInCountInc();
}
