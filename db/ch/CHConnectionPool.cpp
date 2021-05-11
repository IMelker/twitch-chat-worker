//
// Created by imelker on 11.03.2021.
//

#include <algorithm>

#include "nlohmann/json.hpp"

#include "CHConnectionPool.h"

using json = nlohmann::json;

CHConnectionPool::CHConnectionPool(CHConnectionConfig config, unsigned int count,
                                   std::shared_ptr<Logger> logger)
    : DBConnectionPool(count, std::move(logger)), config(std::move(config)) {
    init();
}


std::shared_ptr<DBConnection> CHConnectionPool::createConnection() {
    auto conn = std::make_shared<CHConnection>(this->config, this->logger);

    if (!conn->connected())
        conn.reset();
    else
        all.push_back(conn);

    return conn;
}

std::vector<CHConnection::CHStatistics> CHConnectionPool::collectStats() const {
    std::vector<CHConnection::CHStatistics> stats;
    stats.reserve(all.size());
    std::transform(all.begin(), all.end(), std::back_inserter(stats), [](auto& conn) {
        return conn->getStats();
    });
    return stats;
}
