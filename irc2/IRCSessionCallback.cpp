//
// Created by l2pic on 25.04.2021.
//

#include <cstdarg>
#include <cstdio>
#include "IRCSessionContext.h"
#include "IRCSessionCallback.h"

#define irc_event_callback_to_cxx(function) \
static_cast<IRCSessionContext *>(irc_get_ctx(session))->callback->function(event, origin, std::vector<std::string_view>{params, params + count})
#define irc_event_connected_to_cxx(function) \
static_cast<IRCSessionContext *>(irc_get_ctx(session))->callback->function(event, host)
#define irc_event_disconnected_to_cxx(function) \
static_cast<IRCSessionContext *>(irc_get_ctx(session))->callback->function(event, reason)
#define irc_event_data_to_cxx(function) \
static_cast<IRCSessionContext *>(irc_get_ctx(session))->callback->function(len)
#define irc_event_dcc_chat_to_cxx(function) \
static_cast<IRCSessionContext *>(irc_get_ctx(session))->callback->function(nick, addr, dccid)
#define irc_event_dcc_send_to_cxx(function) \
static_cast<IRCSessionContext *>(irc_get_ctx(session))->callback->function(nick, addr, filename, size, dccid)

static void on_session_log(irc_session_t * session, const char * format, ...) {
    thread_local char buffer[2048] = {};
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);
    static_cast<IRCSessionContext *>(irc_get_ctx(session))->callback->onLog(buffer, len);
}

static void on_connected(irc_session_t *session, const char *event, const char *host) {
    irc_event_connected_to_cxx(onConnected);
}
static void on_disconnected(irc_session_t *session, const char *event, const char *reason) {
    irc_event_disconnected_to_cxx(onDisconnected);
}
static void on_send(irc_session_t *session, int len) {
    irc_event_data_to_cxx(onSendDataLen);
}
static void on_recv(irc_session_t *session, int len) {
    irc_event_data_to_cxx(onRecvDataLen);
}
static void on_login(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onLoggedIn);
}
static void on_nick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onNick);
}
static void on_quit(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onQuit);
}
static void on_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onJoin);
}
static void on_part(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onPart);
}
static void on_mode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onMode);
}
static void on_umode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onUmode);
}
static void on_topic(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onTopic);
}
static void on_kick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onKick);
}
static void on_channel(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onChannel);
}
static void on_privmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onPrivmsg);
}
static void on_notice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onNotice);
}
static void on_channel_notice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onChannelNotice);
}
static void on_invite(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onInvite);
}
static void on_ctcp_req(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onCtcpReq);
}
static void on_ctcp_rep(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onCtcpRep);
}
static void on_ctcp_action(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onCtcpAction);
}
static void on_unknown(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_event_callback_to_cxx(onUnknown);
}
static void on_numeric(irc_session_t *session, unsigned int event, const char *origin, const char ** params, unsigned int count) {
    irc_event_callback_to_cxx(onNumeric);
}
static void on_dcc_chat_req(irc_session_t *session, const char *nick, const char *addr, irc_dcc_t dccid) {
    irc_event_dcc_chat_to_cxx(onDccChatReq);
}
static void on_dcc_send_req(irc_session_t *session, const char *nick, const char *addr, const char * filename, unsigned long size, irc_dcc_t dccid) {
    irc_event_dcc_send_to_cxx(onDccSendReq);
}

IRCSessionCallback::IRCSessionCallback() : irc_callbacks_t{} {
    event_log = on_session_log;
    event_connect = on_connected;
    event_disconnect = on_disconnected;
    event_send_data = on_send;
    event_recv_data = on_recv;
    event_login = on_login;
    event_nick = on_nick;
    event_quit = on_quit;
    event_join = on_join;
    event_part = on_part;
    event_mode = on_mode;
    event_umode = on_umode;
    event_topic = on_topic;
    event_kick = on_kick;
    event_channel = on_channel;
    event_privmsg = on_privmsg;
    event_notice = on_notice;
    event_channel_notice = on_channel_notice;
    event_invite = on_invite;
    event_ctcp_req = on_ctcp_req;
    event_ctcp_rep = on_ctcp_rep;
    event_ctcp_action = on_ctcp_action;
    event_unknown = on_unknown;
    event_numeric = on_numeric;
    event_dcc_chat_req = on_dcc_chat_req;
    event_dcc_send_req = on_dcc_send_req;
}
