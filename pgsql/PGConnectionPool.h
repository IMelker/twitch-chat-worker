//
// Created by l2pic on 07.03.2021.
//

#ifndef CHATSNIFFER_PG_PGCONNECTIONPOOL_H_
#define CHATSNIFFER_PG_PGCONNECTIONPOOL_H_

#include <memory>
#include <mutex>
#include <string>
#include <queue>
#include <condition_variable>
#include <libpq-fe.h>

#include "PGConnection.h"

class Logger;
class PGConnectionPool
{
  public:
    explicit PGConnectionPool(PGConnectionConfig config, unsigned int connectionsCount, std::shared_ptr<Logger> logger);

    std::shared_ptr<PGConnection> lockConnection();
    void unlockConnection(std::shared_ptr<PGConnection> conn);

    const std::shared_ptr<Logger>& getLogger() const { return logger; };

  private:
    std::shared_ptr<Logger> logger;
    PGConnectionConfig config;
    const unsigned int count = 10;

    std::mutex mutex;
    std::condition_variable condition;
    std::queue<std::shared_ptr<PGConnection>> pool;
};

#endif //CHATSNIFFER_PG_PGCONNECTIONPOOL_H_
