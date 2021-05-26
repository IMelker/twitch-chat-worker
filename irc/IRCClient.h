//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCCLIENT_H_
#define CHATCONTROLLER_IRC_IRCCLIENT_H_

#include <libircclient.h>

#include <map>
#include <memory>

#include <so_5/agent.hpp>
#include <so_5/timers.hpp>

#include "IRCMessage.h"
#include "IRCSessionListener.h"
#include "IRCConnectionConfig.h"
#include "IRCClientConfig.h"
#include "IRCChannelList.h"
#include "IRCStatistic.h"
#include "IRCSessionInterface.h"

class Logger;
class DBController;
class IRCSession;
class IRCSelectorPool;
class IRCClient final : public so_5::agent_t,
                        public IRCSessionInterface,
                        public IRCSessionListener
{
  public:
    struct Connect { IRCSession *session = nullptr; mutable int attempt = 0; };
    struct LoggedInCheck { IRCSession *session = nullptr; };
    struct Reload { IRCClientConfig config; };
    struct Shutdown final : so_5::signal_t {};
    struct JoinChannel { std::string channel; };
    struct LeaveChannel { std::string channel; };
    struct SendPING { IRCSession *session = nullptr; std::string host; };
    struct SendMessage { std::string channel; std::string text; };
    struct SendIRC { std::string message; };
  public:
    IRCClient(const context_t &ctx,
              so_5::mbox_t statsCollector,
              so_5::mbox_t processor,
              IRCConnectionConfig conConfig,
              IRCClientConfig cliConfig,
              IRCSelectorPool *pool,
              std::shared_ptr<Logger> logger,
              std::shared_ptr<DBController> db);
    ~IRCClient() override;

    IRCClient(IRCClient&) = delete;
    IRCClient& operator=(IRCClient&) = delete;

    [[nodiscard]] const std::string& nickname() const;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // session events
    void evtConnect(so_5::mhood_t<Connect> evt);
    void evtShutdown(so_5::mhood_t<Shutdown> evt);
    void evtReload(so_5::mhood_t<Reload> evt);
    void evtLoggedInCheck(so_5::mhood_t<LoggedInCheck> evt);
    void evtJoinChannel(so_5::mhood_t<JoinChannel> evt);
    void evtLeaveChannel(so_5::mhood_t<LeaveChannel> evt);
    void evtSendMessage(so_5::mhood_t<SendMessage> message);
    void evtSendIRC(so_5::mhood_t<SendIRC> irc);
    void evtSendPING(so_5::mhood_t<SendPING> ping);

    // IRCSessionInterface implementation
    bool sendQuit(const std::string &reason) override;
    bool sendJoin(const std::string &channel) override;
    bool sendJoin(const std::string &channel, const std::string &key) override;
    bool sendPart(const std::string &channel) override;
    bool sendTopic(const std::string &channel, const std::string &topic) override;
    bool sendNames(const std::string &channel) override;
    bool sendList(const std::string &channel) override;
    bool sendInvite(const std::string &channel, const std::string &nick) override;
    bool sendKick(const std::string &channel, const std::string &nick, const std::string &comment) override;
    bool sendMessage(const std::string &channel, const std::string &text) override;
    bool sendNotice(const std::string &channel, const std::string &text) override;
    bool sendMe(const std::string &channel, const std::string &text) override;
    bool sendChannelMode(const std::string &channel, const std::string &mode) override;
    bool sendCtcpRequest(const std::string &nick, const std::string &reply) override;
    bool sendCtcpReply(const std::string &nick, const std::string &reply) override;
    bool sendUserMode(const std::string &mode) override;
    bool sendNick(const std::string &newnick) override;
    bool sendWhois(const std::string &nick) override;
    bool sendPing(const std::string& host) override;
    bool sendRaw(const std::string &raw) override;

    // implementation IRCSessionListener
    void onLoggedIn(IRCSession* session) override;
    void onDisconnected(IRCSession *session, std::string_view reason) override;
    void onStatistics(IRCSession* session, IRCStatistic&& stats) override;
    void onMessage(IRCMessage &&message) override;
  private:
    void addNewSession();
    void joinToChannel(const std::string &name, IRCSession *session);
    void leaveFromChannel(const std::string &name);
    IRCSession * getNextSessionRoundRobin();
    IRCSession * getNextConnectedSessionRoundRobin();

    so_5::mbox_t statsCollector;
    so_5::mbox_t processor;

    const IRCConnectionConfig conConfig;
    IRCClientConfig cliConfig;
    IRCChannelList channels;

    IRCSelectorPool *pool;
    const std::shared_ptr<Logger> logger;
    std::string loggerTag;

    unsigned int curSessionRoundRobin = 0;
    std::vector<std::shared_ptr<IRCSession>> sessions;
};

#endif //CHATCONTROLLER_IRC_IRCCLIENT_H_
