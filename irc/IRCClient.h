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

#include "IRCEvents.h"
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
                        public IRCStatisticProvider,
                        public IRCSessionInterface,
                        public IRCSessionListener
{
  public:
    struct Connect { IRCSession *session = nullptr; mutable int attempt = 0; };
    struct Reload { IRCClientConfig config; };
  public:
    IRCClient(const context_t &ctx,
              so_5::mbox_t parent,
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

    // IRCStatisticProvider implementation
    [[nodiscard]] const IRCStatistic& statistic() override;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // session events
    void evtConnect(so_5::mhood_t<Connect> evt);

    void evtShutdown(so_5::mhood_t<Irc::Shutdown> evt);
    void evtReload(so_5::mhood_t<Reload> evt);
    void evtJoinChannels(so_5::mhood_t<Irc::JoinChannels> evt);
    void evtJoinChannel(so_5::mhood_t<Irc::JoinChannel> evt);
    void evtLeaveChannel(so_5::mhood_t<Irc::LeaveChannel> evt);
    void evtSendMessage(so_5::mhood_t<Irc::SendMessage> message);
    void evtSendIRC(so_5::mhood_t<Irc::SendIRC> irc);
    void evtSendPING(so_5::mhood_t<Irc::SendPING> ping);

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
    void onDisconnected(IRCSession* session) override;
    void onMessage(IRCMessage &&message) override;
  private:
    void addNewSession();
    void joinToChannel(const std::string &name, IRCSession *session);
    void leaveFromChannel(const std::string &name);
    IRCSession * getNextSessionRoundRobin();

    so_5::mbox_t parent;
    so_5::mbox_t processor;

    const IRCConnectionConfig conConfig;
    IRCClientConfig cliConfig;

    IRCChannelList channels;

    const std::shared_ptr<Logger> logger;
    IRCSelectorPool *pool;

    unsigned int curSessionRoundRobin = 0;
    std::vector<std::shared_ptr<IRCSession>> sessions;

    so_5::timer_id_t pingTimer;
};

#endif //CHATCONTROLLER_IRC_IRCCLIENT_H_
