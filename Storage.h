//
// Created by imelker on 01.04.2021.
//

#ifndef CHATSNIFFER__STORAGE_H_
#define CHATSNIFFER__STORAGE_H_

#include <vector>
#include <memory>

#include "bot/BotLogger.h"

#include "Message.h"
#include "MessageSubscriber.h"

struct LogMessage {
    int userId;
    int botId;
    int handlerId;
    long long int timestamp;
    std::string text;
};

class ThreadPool;
class CHConnectionPool;
class Storage : public MessageSubscriber, public BotLogger
{
  public:
    using MessageBatch = std::vector<std::shared_ptr<Message>>;
    using LogBatch = std::vector<std::unique_ptr<LogMessage>>;
  public:
    explicit Storage(CHConnectionConfig config, int connections, int batchSize,
                     ThreadPool *pool, std::shared_ptr<Logger> logger);
    ~Storage();

    [[nodiscard]] CHConnectionPool *getCHPool() const;

    void store(std::shared_ptr<Message> msg);
    void store(std::unique_ptr<LogMessage>&& msg);

    void flush();

    // implement MessageSubscriber
    void onMessage(std::shared_ptr<Message> message) override;
    // implement BotLogger
    void onLog(int userId, int botId, int handlerId, long long int timestamp, const std::string& text) override;
  private:
    void process(const MessageBatch &batch);
    void process(const LogBatch &batch);

    ThreadPool *pool;
    std::shared_ptr<Logger> logger;
    std::shared_ptr<CHConnectionPool> ch;

    std::mutex logMutex;
    LogBatch logBatch;
    unsigned int logBatchSize = 1000;

    std::mutex msgMutex;
    MessageBatch msgBatch;
    unsigned int batchSize = 1000;
};

#endif //CHATSNIFFER__STORAGE_H_
