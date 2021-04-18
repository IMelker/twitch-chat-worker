//
// Created by imelker on 01.04.2021.
//

#ifndef CHATSNIFFER_IRC_IRCWORKERPOOL_H_
#define CHATSNIFFER_IRC_IRCWORKERPOOL_H_

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <string>

#include <so_5/agent.hpp>

#include "IRCWorker.h"
#include "irc/IRCEvents.h"
#include "HttpControllerEvents.h"

class Logger;
class DBController;
class IRCController final : public so_5::agent_t
{
  public:
    IRCController(const context_t &ctx,
                  so_5::mbox_t processor,
                  so_5::mbox_t http,
                  const IRCConnectConfig &conConfig,
                  std::shared_ptr<DBController> db,
                  std::shared_ptr<Logger> logger);
    ~IRCController() override;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // so_5 events
    void evtSendMessage(mhood_t<SendMessage> message);
    void evtWorkerConnected(mhood_t<WorkerConnected> evt);
    void evtWorkerDisconnected(mhood_t<WorkerDisconnected> evt);
    void evtWorkerLogin(mhood_t<WorkerLoggedIn> evt);

    // http events
    void evtHttpStatus(mhood_t<hreq::irc::stats> evt);
    void evtHttpReload(mhood_t<hreq::irc::reload> evt);
    // channels http handlers
    void evtHttpChannelJoin(mhood_t<hreq::irc::channel::join> evt);
    void evtHttpChannelLeave(mhood_t<hreq::irc::channel::leave> evt);
    void evtHttpChannelMessage(mhood_t<hreq::irc::channel::message> evt);
    void evtHttpChannelCustom(mhood_t<hreq::irc::channel::custom> evt);
    void evtHttpChannelStats(mhood_t<hreq::irc::channel::stats> evt);
    // accounts http handlers
    void evtHttpAccountAdd(mhood_t<hreq::irc::account::add> evt);
    void evtHttpAccountRemove(mhood_t<hreq::irc::account::remove> evt);
    void evtHttpAccountReload(mhood_t<hreq::irc::account::reload> evt);
    void evtHttpAccountStats(mhood_t<hreq::irc::account::stats> evt);

    // implementation HTTPServerUnit
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string& request,
                                                    std::string &error);

    // http handlers
    std::string handleAccounts(const std::string &request, std::string &error);
    std::string handleChannels(const std::string &request, std::string &error);
    std::string handleJoin(const std::string &request, std::string &error);
    std::string handleLeave(const std::string &request, std::string &error);
    std::string handleReloadChannels(const std::string &request, std::string &error);
    std::string handleMessage(const std::string &request, std::string &error);
    std::string handleCustom(const std::string &request, std::string &error);
  private:
    so_5::mbox_t processor;
    so_5::mbox_t http;

    std::shared_ptr<Logger> logger;
    std::shared_ptr<DBController> db;

    const IRCConnectConfig config;
    std::map<std::string, IRCWorker *, std::less<>> workers;

    long long lastChannelLoadTimestamp = 0;
    std::map<std::string, IRCWorker *> watchChannels;
};

#endif //CHATSNIFFER_IRC_IRCWORKERPOOL_H_
