//
// Created by imelker on 01.04.2021.
//

#ifndef CHATSNIFFER__STORAGE_H_
#define CHATSNIFFER__STORAGE_H_

#include <vector>
#include <memory>

#include <so_5/agent.hpp>
#include <so_5/timers.hpp>

#include "bot/BotEvents.h"

#include "db/ch/CHConnection.h"
#include "HttpControllerEvents.h"
#include "ChatMessage.h"

class CHConnectionPool;
class Storage final : public so_5::agent_t
{
  public:
    using ChatMessageHolder = so_5::message_holder_t<Chat::Message>;
    using MessageBatch = std::vector<ChatMessageHolder>;
    using BotLogHolder = so_5::message_holder_t<Bot::LogMessage>;
    using BotLogBatch = std::vector<BotLogHolder>;

    struct Flush final : public so_5::signal_t {};
    struct FlushChatMessages final : public so_5::signal_t {};
    struct FlushBotLogMessages final : public so_5::signal_t {};
    struct GatherStats final : public so_5::signal_t {};
    struct CHPoolMetrics { std::vector<CHConnection::CHStatistics> stats; };
  public:
    explicit Storage(const context_t &ctx,
                     so_5::mbox_t publisher,
                     so_5::mbox_t statsCollector,
                     CHConnectionConfig config,
                     int connections,
                     int batchSize,
                     unsigned int messagesFlushDelay,
                     unsigned int botLogFlushDelay,
                     std::shared_ptr<Logger> logger);
    ~Storage() override;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // so_5 events
    void evtChatMessage(so_5::mhood_t<Chat::Message> msg);
    void evtBotLogMessage(so_5::mhood_t<Bot::LogMessage> msg);
    void evtFlushBotLogMessages(so_5::mhood_t<FlushBotLogMessages> flush);
    void evtFlushChatMessages(so_5::mhood_t<FlushChatMessages> flush);
    void evtFlushAll(so_5::mhood_t<Flush> flush);
    void evtGatherStats(so_5::mhood_t<GatherStats> evt);
  private:
    void store(BotLogHolder &&msg);
    void store(ChatMessageHolder &&msg);

    void process(const MessageBatch &batch);
    void process(const BotLogBatch &batch);
    void flush();

    so_5::mbox_t publisher;
    so_5::mbox_t statsCollector;

    const std::shared_ptr<Logger> logger;
    std::shared_ptr<CHConnectionPool> ch;

    BotLogBatch logBatch;
    std::mutex logBatchMutex;
    so_5::timer_id_t botLogFlushTimer;

    MessageBatch msgBatch;
    std::mutex msgBatchMutex;
    so_5::timer_id_t chatMessageFlushTimer;

    so_5::timer_id_t gatherStatsTimer;

    unsigned int batchSize = 1000;
    unsigned int messagesFlushDelay = 10;
    unsigned int botLogFlushDelay = 10;
};

#endif //CHATSNIFFER__STORAGE_H_
