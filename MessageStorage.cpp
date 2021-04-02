//
// Created by l2pic on 01.04.2021.
//

#include "common/Logger.h"
#include "common/ThreadPool.h"
#include "db/DBConnectionLock.h"
#include "db/ch/CHConnectionPool.h"
#include "MessageStorage.h"

#include <utility>
#include "Message.h"

#define BATCH_SIZE 1000

MessageStorage::MessageStorage(CHConnectionConfig config,
                               unsigned int count,
                               ThreadPool *pool,
                               std::shared_ptr<Logger> logger)
  : pool(pool), logger(std::move(logger)) {
    batch.reserve(BATCH_SIZE);

    this->logger->logInfo("Controller init Clickhouse DB pool with {} connections", count);
    ch = std::make_shared<CHConnectionPool>(std::move(config), count, this->logger);
}

MessageStorage::~MessageStorage() {
    logger->logTrace("MessageStorage end of storage");
    flush();
}


CHConnectionPool *MessageStorage::getCHPool() const {
    return ch.get();
}

void MessageStorage::process(const Batch& messages) {
    if (messages.empty())
        return;

    using namespace clickhouse;
    auto channels = std::make_shared<ColumnFixedString>(256);
    auto user = std::make_shared<ColumnFixedString>(256);
    auto text = std::make_shared<ColumnString>();
    //auto tags = std::make_shared<ColumnString>();
    auto timestamps = std::make_shared<ColumnDateTime64>(3);
    for (const auto & message : messages) {
        channels->Append(message.channel);
        user->Append(message.user);
        text->Append(message.text);
        timestamps->Append(message.timestamp);
    }

    Block block;
    block.AppendColumn("channel", channels);
    block.AppendColumn("from", user);
    block.AppendColumn("text", text);
    block.AppendColumn("timestamp", timestamps);
    try {
        DBConnectionLock chl(ch);
        chl->insert("twitch_chat.messages", block);
        ch->getLogger()->logInfo("Clickhouse insert {} messages", messages.size());
    } catch (const clickhouse::ServerException& err) {
        ch->getLogger()->logError("Clickhouse {}", err.what());
    }
}

void MessageStorage::store(Message &&msg) {
    thread_local Batch tempBatch;
    if (std::lock_guard lg(mutex); batch.size() < BATCH_SIZE) {
        batch.push_back(std::move(msg));
        return;
    } else {
        tempBatch.reserve(BATCH_SIZE);
        tempBatch.swap(batch);
    }

    process(tempBatch);

    tempBatch.clear();
}

void MessageStorage::flush() {
    Batch tempBatch;
    {
        std::lock_guard lg(mutex);
        tempBatch.swap(batch);
    }

    process(tempBatch);

    ch->getLogger()->flush();
}

void MessageStorage::onMessage(const Message &message) {
    pool->enqueue([message = message, this]() mutable {
        store(std::move(message));
    });
}
