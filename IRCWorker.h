//
// Created by imelker on 06.03.2021.
//

#ifndef CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
#define CHATSNIFFER_IRCCLIENT_IRCWORKER_H_

#include <utility>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

#include <so_5/agent.hpp>
#include <so_5/timers.hpp>

#include "common/SysSignal.h"
#include "irc/IRCClient.h"

#include "IRCEvents.h"

struct IRCConnectConfig {
    std::string host;
    int port = 6667;
    int connect_attemps_limit = 30;
};

struct IRCClientConfig {
    int id = 0;
    std::string nick;
    std::string user;
    std::string password;
    int channels_limit = 20;
    int command_per_sec_limit = 20 / 30;
    int whisper_per_sec_limit = 3;
    int auth_per_sec_limit = 2;
};

class Logger;
class IRCWorker final : public so_5::agent_t, public IRCMessageListener
{
    struct {
        struct {
            std::atomic<long long> updated{};
            std::atomic<unsigned int> attempts{};
        } connects;
        struct {
            std::atomic<long long> updated{};
            std::atomic<unsigned int> count{};
        } channels;
        struct {
            struct {
                std::atomic<long long> updated{};
                std::atomic<unsigned int> count{};
            } in;
            struct {
                std::atomic<long long> updated{};
                std::atomic<unsigned int> count{};
            } out;
        } messages;
    } stats;
  public:
    struct Reload { IRCClientConfig config; };
  public:
    explicit IRCWorker(const context_t &ctx,
                       so_5::mbox_t parent,
                       so_5::mbox_t cc,
                       so_5::mbox_t processor,
                       IRCConnectConfig conConfig,
                       IRCClientConfig ircConfig,
                       std::shared_ptr<Logger> logger);
    ~IRCWorker() override;

    IRCWorker(IRCWorker&) = delete;
    IRCWorker& operator=(IRCWorker&) = delete;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    void evtShutdown(so_5::mhood_t<Irc::Shutdown> evt);
    void evtReload(so_5::mhood_t<Reload> evt);
    void evtInitAutoJoin(so_5::mhood_t<Irc::InitAutoJoin> evt);
    void evtJoinChannels(so_5::mhood_t<Irc::JoinChannels> evt);
    void evtJoinChannel(so_5::mhood_t<Irc::JoinChannel> evt);
    void evtLeaveChannels(so_5::mhood_t<Irc::LeaveAllChannels> evt);
    void evtLeaveChannel(so_5::mhood_t<Irc::LeaveChannel> evt);
    void evtSendMessage(so_5::mhood_t<Irc::SendMessage> message);
    void evtSendIRC(so_5::mhood_t<Irc::SendIRC> irc);
    void evtSendPING(so_5::mhood_t<Irc::SendPING> ping);

    [[nodiscard]] const IRCConnectConfig& getConnectConfig() const;
    [[nodiscard]] const IRCClientConfig& getClientConfig() const;
    [[nodiscard]] std::set<std::string> getJoinedChannels() const;
    [[nodiscard]] const decltype(stats)& getStats() const;
    [[nodiscard]] int getFreeChannelSpace() const;

    // implementation IRCMessageListener
    void onMessage(const IRCMessage &message) override;
  private:
    void clearChannels();
    bool sendIRC(const std::string& message);
    void run();

    so_5::mbox_t parent;
    so_5::mbox_t channelController;
    so_5::mbox_t processor;

    const std::shared_ptr<Logger> logger;
    const IRCConnectConfig conConfig;
    IRCClientConfig ircConfig;

    std::unique_ptr<IRCClient> client;

    // TODO remove joinedChannels
    mutable std::mutex channelsMutex;
    std::set<std::string> joinedChannels;

    std::atomic<bool> active, reconnect;
    so_5::timer_id_t pingTimer;
    std::thread thread;
};

#endif //CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
