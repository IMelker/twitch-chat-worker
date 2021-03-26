//
// Created by l2pic on 11.03.2021.
//

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

std::tuple<int, std::string> CHConnectionPool::processHttpRequest(std::string_view path, const std::string &request, std::string &error) {
    if (path == "stats")
        return {200, handleStats(request, error)};

    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}

std::string CHConnectionPool::handleStats(const std::string &request, std::string &error) {
    json body = json::object();
    for(size_t i = 0; i < all.size(); ++i) {
        const auto &stats = all[i]->getStats();
        auto &conn = body[std::to_string(i)] = json::object();
        conn["requests"] = {{"updated", stats.requests.updated.load(std::memory_order_relaxed)},
                            {"count", stats.requests.count.load(std::memory_order_relaxed)},
                            {"failed", stats.requests.failed.load(std::memory_order_relaxed)},
                            {"rtt", stats.requests.rtt.load(std::memory_order_relaxed)}};
    }
    return body.dump();
}

