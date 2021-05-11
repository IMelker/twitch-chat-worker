//
// Created by imelker on 01.04.2021.
//

#include <memory>
#include <chrono>

#include <so_5/send_functions.hpp>

#include "common/Logger.h"
#include "db/DBConnectionLock.h"
#include "db/ch/CHConnectionPool.h"
#include "Storage.h"

#define FLUSH_TIMER(time) std::chrono::seconds{time}, std::chrono::seconds{time}

static constexpr int gatherStatsDelay = 5;

Storage::Storage(const context_t &ctx,
                 so_5::mbox_t publisher,
                 so_5::mbox_t statsCollector,
                 CHConnectionConfig config,
                 int connections,
                 int batchSize,
                 unsigned int messagesFlushDelay,
                 unsigned int botLogFlushDelay,
                 std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx),
    publisher(std::move(publisher)),
    statsCollector(std::move(statsCollector)),
    logger(std::move(logger)),
    batchSize(batchSize),
    messagesFlushDelay(messagesFlushDelay),
    botLogFlushDelay(botLogFlushDelay) {
    msgBatch.reserve(batchSize);
    logBatch.reserve(batchSize);

    this->logger->logInfo("MessageStorage init Clickhouse DB pool with {} connections", connections);
    ch = std::make_shared<CHConnectionPool>(std::move(config), connections, this->logger);
}

Storage::~Storage() {
    logger->logTrace("MessageStorage end of storage");
}

void Storage::so_define_agent() {
    so_subscribe(publisher).event(&Storage::evtChatMessage, so_5::thread_safe);

    so_subscribe_self().event(&Storage::evtBotLogMessage, so_5::thread_safe);
    so_subscribe_self().event(&Storage::evtFlushBotLogMessages, so_5::thread_safe);
    so_subscribe_self().event(&Storage::evtFlushChatMessages, so_5::thread_safe);
    so_subscribe_self().event(&Storage::evtFlushAll, so_5::thread_safe);
    so_subscribe_self().event(&Storage::evtGatherStats, so_5::thread_safe);
}

void Storage::so_evt_start() {
    botLogFlushTimer = so_5::send_periodic<Storage::FlushBotLogMessages>(*this, FLUSH_TIMER(botLogFlushDelay));
    chatMessageFlushTimer = so_5::send_periodic<Storage::FlushChatMessages>(*this, FLUSH_TIMER(messagesFlushDelay));
    gatherStatsTimer = so_5::send_periodic<Storage::GatherStats>(*this, FLUSH_TIMER(gatherStatsDelay));
}

void Storage::so_evt_finish() {
    flush();
}

void Storage::evtChatMessage(so_5::mhood_t<Chat::Message> msg) {
    store(msg.make_holder());
}

void Storage::evtBotLogMessage(so_5::mhood_t<Bot::LogMessage> msg) {
    logger->logTrace("Storage BotLogMessage: timestamp: {}, userId: {}, botId: {}, handlerId: {}, text: \"{}\"",
                     msg->timestamp, msg->userId, msg->botId, msg->handlerId, msg->text);

    store(msg.make_holder());
}

void Storage::evtFlushBotLogMessages(so_5::mhood_t<FlushBotLogMessages>) {
    BotLogBatch temp;
    temp.reserve(batchSize);
    {
        std::lock_guard ul(logBatchMutex);
        temp.swap(logBatch);
    }

    process(temp);
    ch->getLogger()->flush();
}

void Storage::evtFlushChatMessages(so_5::mhood_t<FlushChatMessages>) {
    MessageBatch temp;
    temp.reserve(batchSize);
    {
        std::lock_guard ul(msgBatchMutex);
        temp.swap(msgBatch);
    }

    process(temp);
    ch->getLogger()->flush();
}

void Storage::evtFlushAll(so_5::mhood_t<Flush>) {
    flush();
}

void Storage::flush() {
    MessageBatch tempMsg;
    tempMsg.reserve(batchSize);
    {
        std::lock_guard ul(msgBatchMutex);
        tempMsg.swap(msgBatch);
    }
    process(tempMsg);

    BotLogBatch tempLog;
    tempLog.reserve(batchSize);
    {
        std::lock_guard ul(logBatchMutex);
        tempLog.swap(logBatch);
    }
    process(tempLog);

    ch->getLogger()->flush();
}

void Storage::process(const MessageBatch& messages) {
    if (messages.empty())
        return;

    using namespace clickhouse;
    auto ids = std::make_shared<ColumnUUID>();
    auto channels = std::make_shared<ColumnFixedString>(256);
    auto users = std::make_shared<ColumnFixedString>(256);
    auto texts = std::make_shared<ColumnString>();
    //auto tags = std::make_shared<ColumnString>();
    auto timestamps = std::make_shared<ColumnDateTime64>(3);
    auto languages = std::make_shared<ColumnFixedString>(16);
    for (const auto & message : messages) {
        ids->Append(message->uuid.first);
        channels->Append(message->channel);
        users->Append(message->user);
        texts->Append(message->text);
        timestamps->Append(message->timestamp);
        languages->Append(message->lang);
    }

    Block block;
    block.AppendColumn("id", ids);
    block.AppendColumn("channel", channels);
    block.AppendColumn("from", users);
    block.AppendColumn("text", texts);
    block.AppendColumn("timestamp", timestamps);
    block.AppendColumn("language", languages);
    try {
        DBConnectionLock chl(ch);
        chl->insert("twitch_chat.messages", block);
        ch->getLogger()->logInfo("Clickhouse insert {} messages", messages.size());
    } catch (const clickhouse::ServerException& err) {
        ch->getLogger()->logError("Clickhouse {}", err.what());
    }
}

void Storage::process(const Storage::BotLogBatch &batch) {
    if (batch.empty())
        return;

    using namespace clickhouse;
    auto userIds = std::make_shared<ColumnInt32>();
    auto botIds = std::make_shared<ColumnInt32>();
    auto handlerIds = std::make_shared<ColumnInt32>();
    auto timestamps = std::make_shared<ColumnDateTime64>(3);
    auto text = std::make_shared<ColumnString>();
    auto messageIds = std::make_shared<ColumnUUID>();
    for (const auto & record : batch) {
        userIds->Append(record->userId);
        botIds->Append(record->botId);
        handlerIds->Append(record->handlerId);
        timestamps->Append(record->timestamp);
        text->Append(record->text);
        messageIds->Append(record->messageId);
    }

    Block block;
    block.AppendColumn("user_id", userIds);
    block.AppendColumn("bot_id", botIds);
    block.AppendColumn("handler_id", handlerIds);
    block.AppendColumn("timestamp", timestamps);
    block.AppendColumn("text", text);
    block.AppendColumn("message_id", messageIds);
    try {
        DBConnectionLock chl(ch);
        chl->insert("twitch_chat.user_logs", block);
        ch->getLogger()->logInfo("Clickhouse insert {} log records", batch.size());
    } catch (const clickhouse::ServerException& err) {
        ch->getLogger()->logError("Clickhouse {}", err.what());
    }
}

void Storage::store(ChatMessageHolder &&msg) {
    std::unique_lock ul(msgBatchMutex);
    if (msgBatch.size() < batchSize) {
        msgBatch.push_back(std::move(msg));
    } else {
        MessageBatch temp;
        temp.reserve(batchSize);
        temp.swap(msgBatch);
        ul.unlock();

        process(temp);
    }
}

void Storage::store(BotLogHolder &&msg) {
    std::unique_lock ul(logBatchMutex);
    if (logBatch.size() < batchSize) {
        logBatch.push_back(std::move(msg));
    } else {
        BotLogBatch temp;
        temp.reserve(batchSize);
        temp.swap(logBatch);
        ul.unlock();

        process(temp);
    }
}

void Storage::evtGatherStats(so_5::mhood_t<GatherStats> evt) {
    so_5::send<CHPoolMetrics>(statsCollector, ch->collectStats());
}
