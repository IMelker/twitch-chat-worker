//
// Created by imelker on 07.03.2021.
//

#include <thread>

#include "PGConnectionPool.h"

PGConnectionPool::PGConnectionPool(PGConnectionConfig config, unsigned int count,
                                   std::shared_ptr<Logger> logger)
  : DBConnectionPool(count, std::move(logger)), config(std::move(config)) {
    init();
}

std::shared_ptr<DBConnection> PGConnectionPool::createConnection() {
    auto conn = std::make_shared<PGConnection>(this->config, this->logger);

    if (!conn->connected())
        conn.reset();

    return conn;
}

std::tuple<int, std::string> PGConnectionPool::processHttpRequest(std::string_view path, const std::string &request, std::string &error) {
    if (path == "stats")
        return {200, handleStats(request, error)};

    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}

std::string PGConnectionPool::handleStats(const std::string &request, std::string &error) {
    return std::__cxx11::string();
}
