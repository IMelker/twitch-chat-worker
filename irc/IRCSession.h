//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCSESSION_H_
#define CHATCONTROLLER_IRC_IRCSESSION_H_

#include <libircclient.h>

#include <memory>
#include <mutex>

#include <so_5/timers.hpp>

#include "Logger.h"

#include "IRCConnectionConfig.h"
#include "IRCClientConfig.h"
#include "IRCSessionContext.h"
#include "IRCSessionCallback.h"
#include "IRCSessionInterface.h"
#include "IRCStatistic.h"

class IRCClient;
struct IRCSessionListener;
class IRCSession : public IRCSessionInterface, private IRCSessionCallback
{
  public:
    friend class IRCSelector;
  public:
    IRCSession(const IRCConnectionConfig &conConfig,
               const IRCClientConfig &cliConfig,
               unsigned int id,
               IRCSessionListener *listener,
               IRCClient *parent,
               Logger *logger);
    ~IRCSession() override;

    void create();
    void destroy();
    bool connect();
    bool connected();
    bool loggedIn();
    void disconnect();

    [[nodiscard]] unsigned int getId() const;

    void setPingTimer(so_5::timer_id_t timer);

    // IRCSessionCommands
    bool sendQuit(const std::string& reason) override;
    bool sendJoin(const std::string& channel) override;
    bool sendJoin(const std::string& channel, const std::string& key) override;
    bool sendPart(const std::string& channel) override;
    bool sendTopic(const std::string& channel, const std::string& topic) override;
    bool sendNames(const std::string& channel) override;
    bool sendList(const std::string& channel) override;
    bool sendInvite(const std::string &channel, const std::string &nick) override;
    bool sendKick(const std::string &channel, const std::string &nick, const std::string &comment) override;
    bool sendMessage(const std::string& channel, const std::string& text) override;
    bool sendNotice(const std::string& channel, const std::string& text) override;
    bool sendMe(const std::string& channel, const std::string& text) override;
    bool sendChannelMode(const std::string& channel, const std::string& mode) override;
    bool sendCtcpRequest(const std::string& nick, const std::string& reply) override;
    bool sendCtcpReply(const std::string& nick, const std::string& reply) override;
    bool sendUserMode(const std::string& mode) override;
    bool sendNick(const std::string& newnick) override;
    bool sendWhois(const std::string& nick) override;
    bool sendPing(const std::string &host) override;
    bool sendRaw(const std::string& raw) override;

    // IRCSessionCallback implementation
    void onLog(const char* msg, int len) override;
    void onConnected(std::string_view event, std::string_view host) override;
    void onDisconnected(std::string_view event, std::string_view reason) override;
    void onSendDataLen(int len) override;
    void onRecvDataLen(int len) override;

    void onLoggedIn(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;

    void onNick(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onQuit(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onJoin(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onPart(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onMode(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onUmode(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onTopic(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onKick(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onChannel(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onPrivmsg(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onNotice(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onChannelNotice(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onInvite(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onCtcpReq(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onCtcpRep(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onCtcpAction(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onPong(std::string_view event, std::string_view host) override;
    void onUnknown(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) override;
    void onNumeric(unsigned int event, std::string_view origin, const std::vector<std::string_view>& params) override;

    void onDccChatReq(std::string_view nick, std::string_view addr, unsigned int dccid) override;
    void onDccSendReq(std::string_view nick, std::string_view addr, std::string_view filename, unsigned long size, unsigned int dccid) override;

  private:
    void sendStats(IRCStatistic & stats);
    template<typename Foo, typename ...Args>
    bool internalIrcSend(Foo irc_cmd, Args... args) {
        // IRCClient thread
        if (irc_cmd(session, args...)) {
            logger->logError("IRCSession[{}] Failed to send {}: {}",
                             fmt::ptr(this), __PRETTY_FUNCTION__, irc_strerror(irc_errno(session)));
            return false;
        }
        ++statsFromSo5Thread.commands.out.count;
        return true;
    }

    const IRCConnectionConfig& conConfig;
    const IRCClientConfig& cliConfig;

    unsigned int id = 0;
    IRCSessionContext ctx;

    IRCStatistic statsFromSo5Thread;
    IRCStatistic statsFromSelectorThread;

    IRCSessionListener* listener;
    Logger *logger;
    std::string loggerTag;

    std::atomic_bool logged = false;
    so_5::timer_id_t pingTimer;
    long long lastPingTime = 0;
    long long lastPongTime = 0;

    irc_session_t *session = nullptr;
};

#endif //CHATCONTROLLER_IRC_IRCSESSION_H_
