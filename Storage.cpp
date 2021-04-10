//
// Created by imelker on 01.04.2021.
//

#include "common/Logger.h"
#include "common/ThreadPool.h"
#include "db/DBConnectionLock.h"
#include "db/ch/CHConnectionPool.h"
#include "Storage.h"

#include <memory>
#include <utility>

Storage::Storage(CHConnectionConfig config, int connections,
                 int batchSize, ThreadPool *pool,
                 std::shared_ptr<Logger> logger)
  : pool(pool), logger(std::move(logger)), batchSize(batchSize) {
    msgBatch.reserve(batchSize);

    this->logger->logInfo("MessageStorage init Clickhouse DB pool with {} connections", connections);
    ch = std::make_shared<CHConnectionPool>(std::move(config), connections, this->logger);
}

Storage::~Storage() {
    logger->logTrace("MessageStorage end of storage");
    flush();
}


CHConnectionPool *Storage::getCHPool() const {
    return ch.get();
}

void Storage::process(const MessageBatch& messages) {
    if (messages.empty())
        return;

    using namespace clickhouse;
    auto channels = std::make_shared<ColumnFixedString>(256);
    auto user = std::make_shared<ColumnFixedString>(256);
    auto text = std::make_shared<ColumnString>();
    //auto tags = std::make_shared<ColumnString>();
    auto timestamps = std::make_shared<ColumnDateTime64>(3);
    auto language = std::make_shared<ColumnFixedString>(16);
    for (const auto & message : messages) {
        channels->Append(message->channel);
        user->Append(message->user);
        text->Append(message->text);
        timestamps->Append(message->timestamp);
        language->Append(message->lang);
    }

    Block block;
    block.AppendColumn("channel", channels);
    block.AppendColumn("from", user);
    block.AppendColumn("text", text);
    block.AppendColumn("timestamp", timestamps);
    block.AppendColumn("language", language);
    try {
        DBConnectionLock chl(ch);
        chl->insert("twitch_chat.messages", block);
        ch->getLogger()->logInfo("Clickhouse insert {} messages", messages.size());
    } catch (const clickhouse::ServerException& err) {
        ch->getLogger()->logError("Clickhouse {}", err.what());
    }
}

void Storage::process(const Storage::LogBatch &batch) {
    if (batch.empty())
        return;

    using namespace clickhouse;
    auto userIds = std::make_shared<ColumnInt32>();
    auto botIds = std::make_shared<ColumnInt32>();
    auto handlerIds = std::make_shared<ColumnInt32>();
    auto timestamps = std::make_shared<ColumnDateTime64>(3);
    auto text = std::make_shared<ColumnString>();
    for (const auto & record : batch) {
        userIds->Append(record->userId);
        botIds->Append(record->botId);
        handlerIds->Append(record->handlerId);
        timestamps->Append(record->timestamp);
        text->Append(record->text);
    }

    Block block;
    block.AppendColumn("user_id", userIds);
    block.AppendColumn("bot_id", botIds);
    block.AppendColumn("handler_id", handlerIds);
    block.AppendColumn("timestamp", timestamps);
    block.AppendColumn("text", text);
    try {
        DBConnectionLock chl(ch);
        chl->insert("twitch_chat.user_logs", block);
        ch->getLogger()->logInfo("Clickhouse insert {} log records", batch.size());
    } catch (const clickhouse::ServerException& err) {
        ch->getLogger()->logError("Clickhouse {}", err.what());
    }
}

void Storage::store(std::shared_ptr<Message> msg) {
    thread_local MessageBatch tempBatch;
    if (std::lock_guard lg(msgMutex); msgBatch.size() < batchSize) {
        msgBatch.push_back(std::move(msg));
        return;
    } else {
        tempBatch.reserve(batchSize);
        tempBatch.swap(msgBatch);
    }

    process(tempBatch);

    tempBatch.clear();
}

void Storage::store(std::unique_ptr<LogMessage> &&msg) {
    thread_local LogBatch tempBatch;
    if (std::lock_guard lg(logMutex); logBatch.size() < batchSize) {
        logBatch.push_back(std::move(msg));
        return;
    } else {
        tempBatch.reserve(batchSize);
        tempBatch.swap(logBatch);
    }

    process(tempBatch);

    tempBatch.clear();
}

void Storage::flush() {
    MessageBatch msgTempBatch;
    {
        std::lock_guard lg(msgMutex);
        msgTempBatch.swap(msgBatch);
    }
    process(msgTempBatch);

    LogBatch logTempBatch;
    {
        std::lock_guard lg(logMutex);
        logTempBatch.swap(logBatch);
    }
    process(logTempBatch);

    ch->getLogger()->flush();
}

void Storage::onMessage(std::shared_ptr<Message> message) {
    pool->enqueue([message = std::move(message), this]() mutable {
        store(std::move(message));
    });
}

void Storage::onLog(int userId, int botId, int handlerId, long long int timestamp, const std::string& text) {
    auto message = std::make_unique<LogMessage>(LogMessage{userId, botId, handlerId, timestamp, text});
    pool->enqueue([message = std::move(message), this]() mutable {
        store(std::move(message));
    });
}
