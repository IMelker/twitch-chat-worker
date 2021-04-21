//
// Created by l2pic on 19.04.2021.
//

#ifndef CHATCONTROLLER__CHANNELCONTROLLER_H_
#define CHATCONTROLLER__CHANNELCONTROLLER_H_

#include <string>
#include <map>

#include <so_5/agent.hpp>
#include <so_5/mbox.hpp>

#include "IRCController.h"
#include "IRCEvents.h"

class IRCWorker;
class DBController;
class Logger;
class Channel {
  public:
    explicit Channel(std::string id) : name(std::move(id)) {
    };
    ~Channel() = default;

    void attach(IRCWorker *irc) { worker = irc; }
    void detach() { worker = nullptr; }
    [[nodiscard]] bool attachedTo(IRCWorker *irc) const { return worker == irc; }
    [[nodiscard]] bool attached() const { return worker != nullptr; }

    [[nodiscard]] const std::string& getName() const { return name; }
    [[nodiscard]] const so_5::mbox_t& getWorkerMbox() const { return worker->so_direct_mbox(); }
  private:
    std::string name;
    IRCWorker *worker = nullptr;
};

// ChannelController must be attached to parent thread
class ChannelController : public so_5::agent_t
{
  public:
    explicit ChannelController(const context_t &ctx,
                               const IRCController::WorkersByName &workers,
                               std::shared_ptr<DBController> db,
                               std::shared_ptr<Logger> logger);
    ~ChannelController() override;

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // IRCWorker events
    void evtDetachWorker(so_5::mhood_t<Irc::WorkerDisconnected> evt);
    void evtGetChannels(so_5::mhood_t<Irc::GetChannels> evt);

    // IRCController events
    void evtReload(so_5::mhood_t<Irc::ReloadChannels> evt);
    void evtJoin(so_5::mhood_t<Irc::JoinChannel> evt);
    void evtLeave(so_5::mhood_t<Irc::LeaveChannel> evt);
    void evtJoinOnAccount(so_5::mhood_t<Irc::JoinChannelOnAccount> evt);

  private:
    const std::shared_ptr<Logger> logger;
    const std::shared_ptr<DBController> db;

    const IRCController::WorkersByName& workers;
    std::map<std::string, Channel> channels;
};

#endif //CHATCONTROLLER__CHANNELCONTROLLER_H_
