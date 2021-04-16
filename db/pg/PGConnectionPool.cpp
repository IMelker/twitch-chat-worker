//
// Created by imelker on 07.03.2021.
//

#include <thread>

#include "nlohmann/json.hpp"

#include "PGConnectionPool.h"

using json = nlohmann::json;

PGConnectionPool::PGConnectionPool(PGConnectionConfig config, unsigned int count,
                                   std::shared_ptr<Logger> logger)
  : DBConnectionPool(count, std::move(logger)), config(std::move(config)) {
    init();
}

std::shared_ptr<DBConnection> PGConnectionPool::createConnection() {
    auto conn = std::make_shared<PGConnection>(this->config, this->logger);

    if (!conn->connected())
        conn.reset();
    else
        all.push_back(conn);

    return conn;
}

std::string PGConnectionPool::httpStats(const std::string &request, std::string &error) {
    json body = json::object();

    for(size_t i = 0; i < all.size(); ++i) {
        const auto &stats = all[i]->getStats();
        auto &conn = body[std::to_string(i)] = json::object();
        conn["reconnects"] = {{"updated", stats.connects.updated.load(std::memory_order_relaxed)},
                              {"attempts", stats.connects.attempts.load(std::memory_order_relaxed)}};
        conn["requests"] = {{"updated", stats.requests.updated.load(std::memory_order_relaxed)},
                            {"count", stats.requests.count.load(std::memory_order_relaxed)},
                            {"failed", stats.requests.failed.load(std::memory_order_relaxed)},
                            {"rtt", stats.requests.rtt.load(std::memory_order_relaxed)}};
    }

    return body.dump();
}
