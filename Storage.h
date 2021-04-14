//
// Created by imelker on 01.04.2021.
//

#ifndef CHATSNIFFER__STORAGE_H_
#define CHATSNIFFER__STORAGE_H_

#include <vector>
#include <memory>

#include <so_5/agent.hpp>
#include <so_5/timers.hpp>

#include "bot/BotLogger.h"

#include "ChatMessage.h"

class CHConnectionPool;
class Storage final : public so_5::agent_t
{
  public:
    using ChatMessageHolder = so_5::message_holder_t<Chat::Message>;
    using MessageBatch = std::vector<ChatMessageHolder>;
    using BotLogHolder = so_5::message_holder_t<BotLogger::Message>;
    using BotLogBatch = std::vector<BotLogHolder>;

    struct Flush final : public so_5::signal_t {};
    struct FlushChatMessages final : public so_5::signal_t {};
    struct FlushBotLogMessages final : public so_5::signal_t {};
  public:
    explicit Storage(const context_t &ctx,
                     so_5::mbox_t publisher,
                     CHConnectionConfig config,
                     int connections,
                     int batchSize,
                     std::shared_ptr<Logger> logger);
    ~Storage() override;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // so_5 events
    void evtChatMessage(mhood_t<Chat::Message> msg);
    void evtBotLogMessage(mhood_t<BotLogger::Message> msg);
    void evtFlushBotLogMessages(mhood_t<FlushBotLogMessages> flush);
    void evtFlushChatMessages(mhood_t<FlushChatMessages> flush);
    void evtFlushAll(mhood_t<Flush> flush);
  private:
    void store(BotLogHolder &&msg);
    void store(ChatMessageHolder &&msg);

    void process(const MessageBatch &batch);
    void process(const BotLogBatch &batch);
    void flush();

    so_5::mbox_t publisher;

    std::shared_ptr<Logger> logger;
    std::shared_ptr<CHConnectionPool> ch;

    BotLogBatch logBatch;
    std::mutex logBatchMutex;
    so_5::timer_id_t botLogFlushTimer;

    MessageBatch msgBatch;
    std::mutex msgBatchMutex;
    so_5::timer_id_t chatMessageFlushTimer;

    unsigned int batchSize = 1000;
};

#endif //CHATSNIFFER__STORAGE_H_
