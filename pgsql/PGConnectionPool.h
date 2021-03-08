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

#define CONN_COUNT 10

class PGConnectionPool
{
  public:
    explicit PGConnectionPool(PGConnectionConfig config, unsigned int connectionsCount = CONN_COUNT);

    std::shared_ptr<PGConnection> lockConnection();
    void unlockConnection(std::shared_ptr<PGConnection> conn);

  private:
    void init();

    PGConnectionConfig config;
    const unsigned int count = 10;

    std::mutex mutex;
    std::condition_variable condition;
    std::queue<std::shared_ptr<PGConnection>> pool;
};

#endif //CHATSNIFFER_PG_PGCONNECTIONPOOL_H_
