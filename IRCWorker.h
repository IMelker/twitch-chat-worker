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
#include "irc/IRCEvents.h"

struct IRCConnectConfig {
    std::string host;
    int port = 6667;
    int connect_attemps_limit = 30;
};

struct IRCClientConfig {
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
    explicit IRCWorker(const context_t &ctx,
                       so_5::mbox_t parent,
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

    void evtJoinChannel(mhood_t<JoinChannel> evt);
    void evtLeaveChannel(mhood_t<LeaveChannel> evt);
    void evtSendMessage(mhood_t<SendMessage> message);
    void evtSendIRC(mhood_t<SendIRC> irc);
    void evtSendPING(mhood_t<SendPING> ping);

    [[nodiscard]] const IRCConnectConfig& getConnectConfig() const;
    [[nodiscard]] const IRCClientConfig& getClientConfig() const;
    [[nodiscard]] std::set<std::string> getJoinedChannels() const;
    [[nodiscard]] const decltype(stats)& getStats() const;

    int freeChannelSpace() const;

    // implementation IRCMessageListener
    void onMessage(const IRCMessage &message) override;
  private:
    bool sendIRC(const std::string& message);
    void run();

    const std::shared_ptr<Logger> logger;

    so_5::mbox_t parent;
    so_5::mbox_t processor;

    const IRCConnectConfig conConfig;
    const IRCClientConfig ircConfig;

    std::unique_ptr<IRCClient> client;

    mutable std::mutex channelsMutex;
    std::set<std::string> joinedChannels;

    so_5::timer_id_t pingTimer;
    std::thread thread;
};

#endif //CHATSNIFFER_IRCCLIENT_IRCWORKER_H_
