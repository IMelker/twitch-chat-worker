//
// Created by imelker on 07.03.2021.
//

#include <thread>

#include "PGConnectionPool.h"

PGConnectionPool::PGConnectionPool(PGConnectionConfig config,
                                   unsigned int connectionsCount,
                                   std::shared_ptr<Logger> logger)
  : logger(std::move(logger)), config(std::move(config)), count(connectionsCount) {
    std::lock_guard<std::mutex> lg(mutex);
    for (unsigned int i = 0; i < count; ++i) {
        pool.emplace(std::make_shared<PGConnection>(this->config, this->logger));
    }
}

std::shared_ptr<PGConnection> PGConnectionPool::lockConnection() {
    std::unique_lock<std::mutex> ul(mutex);
    while (pool.empty()) {
        condition.wait(ul);
    }

    auto conn = pool.front();
    pool.pop();

    return conn;
}

void PGConnectionPool::unlockConnection(std::shared_ptr<PGConnection> conn) {
    {
        std::lock_guard<std::mutex> lg(mutex);
        pool.push(std::move(conn));
    }
    condition.notify_one();
}

