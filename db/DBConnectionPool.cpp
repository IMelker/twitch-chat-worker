//
// Created by l2pic on 12.03.2021.
//

#include <thread>

#include "DBConnectionPool.h"

DBConnectionPool::DBConnectionPool(unsigned int count, std::shared_ptr<Logger> logger)
    : logger(std::move(logger)), count(count) {

}

DBConnectionPool::~DBConnectionPool() = default;

void DBConnectionPool::init() {
    std::lock_guard<std::mutex> lg(mutex);
    for (unsigned int i = 0; i < count; ++i) {
        pool.emplace(createConnection());
    }
}

std::shared_ptr<DBConnection> DBConnectionPool::lockConnection() {
    std::unique_lock<std::mutex> ul(mutex);
    while (pool.empty()) {
        condition.wait(ul);
    }

    auto conn = pool.front();
    pool.pop();

    return conn;
}

void DBConnectionPool::unlockConnection(std::shared_ptr<DBConnection> conn) {
    {
        std::lock_guard<std::mutex> lg(mutex);
        pool.push(std::move(conn));
    }
    condition.notify_one();
}
