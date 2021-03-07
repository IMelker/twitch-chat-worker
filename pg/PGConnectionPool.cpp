//
// Created by imelker on 07.03.2021.
//

#include <thread>

#include "PGConnectionPool.h"

PGConnectionPool::PGConnectionPool(PGConnectionConfig config, int connectionsCount)
  : config(std::move(config)), count(connectionsCount) {
    init();
}

void PGConnectionPool::init() {
    std::lock_guard<std::mutex> lg(mutex);
    for (auto i = 0; i < count; ++i) {
        pool.emplace(std::make_shared<PGConnection>(config));
    }
}

std::shared_ptr<PGConnection> PGConnectionPool::lockConnection() {
    std::unique_lock<std::mutex> ul(mutex);
    condition.wait(ul, [&]{return pool.empty();});

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

