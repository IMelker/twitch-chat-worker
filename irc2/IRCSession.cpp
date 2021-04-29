//
// Created by l2pic on 25.04.2021.
//

#include "Clock.h"
#include "IRCSession.h"

IRCSession::IRCSession(const IRCConnectionConfig &conConfig, const IRCClientConfig &cliConfig, IRCClient *client)
    : conConfig(conConfig), cliConfig(cliConfig) {
    session = irc_create_session(this);

    ctx.callback = this;
    ctx.parent = client;

    irc_set_ctx(session, &ctx);

    irc_option_set(session, LIBIRC_OPTION_DEBUG);
    irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);
}

IRCSession::~IRCSession() {
    irc_destroy_session(session);
}

const IRCStatistic & IRCSession::statistic() const {
    return stats;
}

bool IRCSession::connect() {
    // IRCClient thread
    if (irc_connect(session,
                    conConfig.host.c_str(), conConfig.port,
                    cliConfig.password.c_str(), cliConfig.nick.c_str(), cliConfig.user.c_str(), "IRC Client")) {
        stats.connectsFailedInc();
        fprintf(stderr, "IRCSession Could not connect: %s\n", irc_strerror(irc_errno(session)));
        return false;
    }
    stats.connectsSuccessInc();
    stats.connectsUpdatedSet(CurrentTime<std::chrono::system_clock>::milliseconds());
    return true;
}

bool IRCSession::connected() {
    // IRCWorker thread
    return irc_is_connected(session);
}

void IRCSession::disconnect() {
    // IRCClient & IRCWorker thread
    printf("IRCSession Session(%p) disconnect\n", this);
    irc_disconnect(session);
}

bool IRCSession::sendQuit(const std::string &reason) {
    return internalIrcSend(irc_cmd_quit, reason.c_str());
}

bool IRCSession::sendJoin(const std::string &channel) {
    stats.channelsCountInc();
    return internalIrcSend(irc_cmd_join, channel.c_str(), nullptr);
}

bool IRCSession::sendJoin(const std::string &channel, const std::string &key) {
    stats.channelsCountInc();
    return internalIrcSend(irc_cmd_join, channel.c_str(), key.c_str());
}

bool IRCSession::sendPart(const std::string &channel) {
    stats.channelsCountDec();
    return internalIrcSend(irc_cmd_part, channel.c_str());
}

bool IRCSession::sendTopic(const std::string &channel, const std::string &topic) {
    return internalIrcSend(irc_cmd_topic, channel.c_str(), topic.c_str());
}

bool IRCSession::sendNames(const std::string &channel) {
    return internalIrcSend(irc_cmd_names, channel.c_str());
}

bool IRCSession::sendList(const std::string &channel) {
    return internalIrcSend(irc_cmd_list, channel.c_str());
}

bool IRCSession::sendInvite(const std::string &channel, const std::string &nick) {
    return internalIrcSend(irc_cmd_invite, nick.c_str(), channel.c_str());
}

bool IRCSession::sendKick(const std::string &channel, const std::string &nick, const std::string &comment) {
    return internalIrcSend(irc_cmd_kick, nick.c_str(), channel.c_str(), comment.c_str());
}

bool IRCSession::sendMessage(const std::string &channel, const std::string &text) {
    return internalIrcSend(irc_cmd_msg, channel.c_str(), text.c_str());
}

bool IRCSession::sendNotice(const std::string &channel, const std::string &text) {
    return internalIrcSend(irc_cmd_notice, channel.c_str(), text.c_str());
}

bool IRCSession::sendCtcpRequest(const std::string &nick, const std::string &reply) {
    return internalIrcSend(irc_cmd_ctcp_request, nick.c_str(), reply.c_str());
}

bool IRCSession::sendCtcpReply(const std::string &nick, const std::string &reply) {
    return internalIrcSend(irc_cmd_ctcp_reply, nick.c_str(), reply.c_str());
}

bool IRCSession::sendMe(const std::string &channel, const std::string &text) {
    return internalIrcSend(irc_cmd_me, channel.c_str(), text.c_str());
}

bool IRCSession::sendChannelMode(const std::string &channel, const std::string &mode) {
    return internalIrcSend(irc_cmd_channel_mode, channel.c_str(), mode.c_str());
}

bool IRCSession::sendUserMode(const std::string &mode) {
    return internalIrcSend(irc_cmd_user_mode, mode.c_str());
}

bool IRCSession::sendNick(const std::string &newnick) {
    return internalIrcSend(irc_cmd_nick, newnick.c_str());
}

bool IRCSession::sendWhois(const std::string &nick) {
    return internalIrcSend(irc_cmd_nick, nick.c_str());
}

bool IRCSession::sendPing(const std::string &host) {
    return internalIrcSend(irc_cmd_ping, host.c_str());
}

bool IRCSession::sendRaw(const std::string &raw) {
    return internalIrcSend(irc_send_raw, raw.c_str());
}

void IRCSession::onLog(const char *msg, int len) {
    // IRCWorker thread
    //printf("%*s", len, msg);
}

void IRCSession::onConnected(std::string_view event, std::string_view host) {
    // IRCWorker thread
    printf("IRCSession On connected event handler: %s %s\n", event.data(), host.data());
}

void IRCSession::onDisconnected(std::string_view event, std::string_view reason) {
    // IRCWorker thread
    printf("IRCSession On disconnected event handler: %s %s\n", event.data(), reason.data());

    // clear internal session data
    disconnect();

    stats.connectsDisconnectedInc();
}

void IRCSession::onSendDataLen(int len) {
    // IRCWorker thread
    stats.commandsOutBytesAdd(len);
}

void IRCSession::onRecvDataLen(int len) {
    // IRCWorker thread
    stats.commandsInBytesAdd(len);

    printf("%s\n", stats.dump().c_str());
}

void IRCSession::onLoggedIn(std::string_view event,
                            std::string_view origin,
                            const std::vector<std::string_view> &params) {
    // IRCWorker thread
    printf("IRCSession On connect event handler: %s %s\n", event.data(), origin.data());
    stats.connectsLoggedInInc();

    sendJoin("#quickhuntik");
}

void IRCSession::onNick(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);

}

void IRCSession::onQuit(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onJoin(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onPart(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onMode(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    internaIrcRecv(event, origin);
}

void IRCSession::onUmode(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onTopic(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onKick(std::string_view event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onChannel(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);

    for(auto &data: params) {
        printf(" %s", data.data());
    }
    printf("\n");
}

void IRCSession::onPrivmsg(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);

    for(auto &data: params) {
        printf(" %s", data.data());
    }
    printf("\n");
}

void IRCSession::onNotice(std::string_view event,
                          std::string_view origin,
                          const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onChannelNotice(std::string_view event,
                                 std::string_view origin,
                                 const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onInvite(std::string_view event,
                          std::string_view origin,
                          const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onCtcpReq(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onCtcpRep(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onCtcpAction(std::string_view event,
                              std::string_view origin,
                              const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onPong(std::string_view event, std::string_view host) {
    // IRCWorker thread
    internaIrcRecv(event, host);
}


void IRCSession::onUnknown(std::string_view event,
                           std::string_view origin,
                           const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(event, origin);
}

void IRCSession::onNumeric(unsigned int event, std::string_view origin, const std::vector<std::string_view> &params) {
    // IRCWorker thread
    internaIrcRecv(std::to_string(event), origin);
}

void IRCSession::onDccChatReq(std::string_view nick, std::string_view addr, unsigned int dccid) {
    // IRCWorker thread
    internaIrcRecv("DCC_CHAT", nick);
}

void IRCSession::onDccSendReq(std::string_view nick,
                              std::string_view addr,
                              std::string_view filename,
                              unsigned long size,
                              unsigned int dccid) {
    // IRCWorker thread
    internaIrcRecv("DCC_SEND", nick);
}
