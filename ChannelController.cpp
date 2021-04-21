//
// Created by l2pic on 19.04.2021.
//

#include <so_5/send_functions.hpp>

#include "common/Logger.h"
#include "DBController.h"
#include "IRCWorker.h"

#include "ChannelController.h"

inline void fillChannelsMap(const std::shared_ptr<DBController> db, std::map<std::string, Channel>& target) {
    auto list = db->loadChannels();
    std::transform(list.begin(), list.end(), std::inserter(target, target.end()), [] (auto& name) {
        return std::make_pair(name, Channel{name});
    });
}

ChannelController::ChannelController(const context_t &ctx,
                                     const IRCController::WorkersByName &workers,
                                     std::shared_ptr<DBController> db,
                                     std::shared_ptr<Logger> logger)
    : so_5::agent_t(ctx), logger(std::move(logger)), db(std::move(db)), workers(workers) {
    fillChannelsMap(this->db, channels);
}

ChannelController::~ChannelController() = default;

void ChannelController::so_define_agent() {
    so_subscribe_self().event(&ChannelController::evtDetachWorker);
    so_subscribe_self().event(&ChannelController::evtGetChannels);
    so_subscribe_self().event(&ChannelController::evtReload);
    so_subscribe_self().event(&ChannelController::evtJoin);
    so_subscribe_self().event(&ChannelController::evtLeave);
    so_subscribe_self().event(&ChannelController::evtJoinOnAccount);
}

void ChannelController::so_evt_start() {

}

void ChannelController::so_evt_finish() {

}

void ChannelController::evtDetachWorker(so_5::mhood_t<Irc::WorkerDisconnected> evt) {
    for (auto &[name, channel]: channels) {
        if (channel.attachedTo(evt->worker))
            channel.detach();
    }
}

void ChannelController::evtGetChannels(so_5::mhood_t<Irc::GetChannels> evt) {
    std::vector<std::string> channelsList;
    channelsList.reserve(evt->count);

    for (auto &[name, channel]: channels) {
        if (!channel.attached()) {
            channelsList.push_back(name);
            channel.attach(evt->worker);
        }
        if (channelsList.size() >= evt->count)
            break;
    }

    so_5::send<Irc::JoinChannels>(evt->worker->so_direct_mbox(), std::move(channelsList));
}

void ChannelController::evtReload(so_5::mhood_t<Irc::ReloadChannels> evt) {
    // refill channels with new list
    channels.clear();
    fillChannelsMap(db, channels);

    // leave from all channels and init auto join
    for (auto &[account, worker]: workers) {
        so_5::send<Irc::LeaveAllChannels>(worker->so_direct_mbox());
        so_5::send<Irc::InitAutoJoin>(worker->so_direct_mbox());
    }
}

void ChannelController::evtJoin(so_5::mhood_t<Irc::JoinChannel> evt) {

}

void ChannelController::evtLeave(so_5::mhood_t<Irc::LeaveChannel> evt) {
    auto it = channels.find(evt->channel);
    if (it == channels.end())
        return;

    if (it->second.attached())
        so_5::send(it->second.getWorkerMbox(), evt);
}

void ChannelController::evtJoinOnAccount(so_5::mhood_t<Irc::JoinChannelOnAccount> evt) {
    auto it = channels.find(evt->channel);
    if (it == channels.end()) {
        it = channels.emplace(evt->channel, Channel{evt->channel}).first;
    }

    auto& channel = it->second;
    channel.attach(evt->worker);

    so_5::send<Irc::JoinChannel>(channel.getWorkerMbox(), evt->channel);
}
