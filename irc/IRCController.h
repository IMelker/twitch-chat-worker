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
#include <so_5/disp/thread_pool/pub.hpp>

#include "../HttpControllerEvents.h"
#include "../ChatMessage.h"

#include "IRCConnectionConfig.h"
#include "IRCClient.h"
#include "IRCSelectorPool.h"

class Logger;
class DBController;
class ChannelController;
class IRCController final : public so_5::agent_t
{
  public:
    using IRCClientsByName = std::map<std::string, IRCClient *, std::less<>>;
    using IRCClientsByIds = std::map<int, IRCClient *>;

  public:
    IRCController(const context_t &ctx,
                  so_5::mbox_t processor,
                  so_5::mbox_t statsCollector,
                  so_5::mbox_t http,
                  const IRCConnectionConfig &conConfig,
                  std::shared_ptr<DBController> db,
                  std::shared_ptr<Logger> logger);
    ~IRCController() override;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // SendMessage from BotEngine
    void evtSendMessage(so_5::mhood_t<Chat::SendMessage> message);

    // http events
    void evtHttpReload(so_5::mhood_t<hreq::irc::reload> evt);
    void evtHttpCustom(so_5::mhood_t<hreq::irc::custom> evt);
    // channels http handlers
    void evtHttpChannelJoin(so_5::mhood_t<hreq::irc::channel::join> evt);
    void evtHttpChannelLeave(so_5::mhood_t<hreq::irc::channel::leave> evt);
    void evtHttpChannelMessage(so_5::mhood_t<hreq::irc::channel::message> evt);
    // accounts http handlers
    void evtHttpAccountAdd(so_5::mhood_t<hreq::irc::account::add> evt);
    void evtHttpAccountRemove(so_5::mhood_t<hreq::irc::account::remove> evt);
    void evtHttpAccountReload(so_5::mhood_t<hreq::irc::account::reload> evt);
  private:
    IRCClient * getIrcClient(const std::string& name);
    IRCClient * getIrcClient(int id);
    void addNewIrcClient(const IRCClientConfig& config);

    so_5::mbox_t processor;
    so_5::mbox_t statsCollector;
    so_5::mbox_t http;

    so_5::disp::thread_pool::dispatcher_handle_t ircSendPool;
    so_5::disp::thread_pool::bind_params_t ircSendPoolParams;

    const std::shared_ptr<Logger> logger;
    const std::shared_ptr<DBController> db;

    const IRCConnectionConfig config;

    IRCSelectorPool pool;

    IRCClientsByName ircClientsByName;
    IRCClientsByIds ircClientsById;
};

#endif //CHATSNIFFER_IRC_IRCWORKERPOOL_H_
