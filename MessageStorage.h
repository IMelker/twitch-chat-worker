//
// Created by imelker on 01.04.2021.
//

#ifndef CHATSNIFFER__STORAGE_H_
#define CHATSNIFFER__STORAGE_H_

#include <vector>

#include "MessageSubscriber.h"

class ThreadPool;
class CHConnectionPool;
class MessageStorage : public MessageSubscriber
{
    using Batch = std::vector<Message>;
  public:
    explicit MessageStorage(CHConnectionConfig config, unsigned int count, ThreadPool *pool, std::shared_ptr<Logger> logger);
    ~MessageStorage();

    [[nodiscard]] CHConnectionPool *getCHPool() const;

    void store(Message &&msg);

    void flush();

    // implement IRCMessageSubscriber
    void onMessage(const Message &message) override;
  private:
    void process(const Batch& batch);

    ThreadPool *pool;
    std::shared_ptr<Logger> logger;
    std::shared_ptr<CHConnectionPool> ch;

    std::mutex mutex;
    Batch batch;
};

#endif //CHATSNIFFER__STORAGE_H_
