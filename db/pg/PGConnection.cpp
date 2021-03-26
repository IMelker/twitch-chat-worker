//
// Created by imelker on 07.03.2021.
//

#include <utility>
#include <thread>
#include <chrono>

#include "spdlog/fmt/ostr.h"

#include "PGConnection.h"

#include "../../common/Logger.h"
#include "../../common/Clock.h"

PGConnection::PGConnection(PGConnectionConfig  config, std::shared_ptr<Logger> logger)
  : DBConnection(std::move(logger)), config(std::move(config)) {
    for (unsigned int i = 0; ; ) {
        if (resetConnect()) {
            established = true;
            return;
        } else {
            if (++i > config.sendRetries)
                return;
            std::this_thread::sleep_for(std::chrono::seconds{config.retryDelaySec});
        }
    }
}

bool PGConnection::request(const std::string &request) {
    logger->logTrace("PGConnection request: \"{}\"", request);

    auto first = CurrentTime<std::chrono::system_clock>::milliseconds();

    bool success = false;
    PQsendQuery(raw(), request.c_str());
    while (auto resp = PQgetResult(raw())) {
        auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.requests.rtt.store(now - first, std::memory_order_relaxed);
        stats.requests.updated.store(now, std::memory_order_relaxed);

        auto status = PQresultStatus(resp);
        if (status == PGRES_TUPLES_OK) {
            success = true;
        }
        if (status == PGRES_FATAL_ERROR) {
            logger->logError("PGConnection Error: {}", PQresultErrorMessage(resp));
            stats.requests.failed.fetch_add(1, std::memory_order_relaxed);
            success = false;
        }
        PQclear(resp);
    }

    stats.requests.count.fetch_add(1, std::memory_order_relaxed);
    return success;
}

bool PGConnection::request(const std::string &request, std::vector<std::vector<std::string>>& result) {
    logger->logTrace("PGConnection request: \"{}\"", request);

    auto first = CurrentTime<std::chrono::system_clock>::milliseconds();

    bool success = false;
    PQsendQuery(raw(), request.c_str());
    while (auto resp = PQgetResult(raw())) {
        auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
        stats.requests.rtt.store(now - first, std::memory_order_relaxed);
        stats.requests.updated.store(now, std::memory_order_relaxed);

        auto status = PQresultStatus(resp);
        if (status == PGRES_TUPLES_OK) {
            success = true;

            int rows = PQntuples(resp);
            int columns = PQnfields(resp);

            result.reserve(rows);
            for (int i = 0; i < rows; ++i) {
                std::vector<std::string> row;
                row.reserve(columns);
                for (int j = 0; j < columns; j++) {
                    row.emplace_back(PQgetvalue(resp, i, j),
                                     static_cast<size_t>(PQgetlength(resp, i, j)));
                }
                result.push_back(std::move(row));
            }

            logger->logInfo("PGConnection Request result: rows: {} columns: {}", rows, columns);
        }

        if (status == PGRES_FATAL_ERROR) {
            logger->logError("PGConnection Error: {}", PQresultErrorMessage(resp));
            stats.requests.failed.fetch_add(1, std::memory_order_relaxed);
            success = false;
        }

        PQclear(resp);
    }
    return success;
}

bool PGConnection::ping() {
    return retryGuard([this]() {
        return request("SELECT 1;");
    });
}

bool PGConnection::resetConnect() {
    stats.connects.attempts.fetch_add(1, std::memory_order_relaxed);

    conn.reset(PQsetdbLogin(config.host.c_str(), std::to_string(config.port).c_str(), nullptr, nullptr,
                            config.dbname.c_str(), config.user.c_str(), config.pass.c_str()), &PQfinish);

    if (PQstatus(conn.get()) != CONNECTION_OK && PQsetnonblocking(conn.get(), 1) != 0) {
        this->logger->logCritical("PGConnection {}:{} {}", config.host, config.port, PQerrorMessage(conn.get()));
        return false;
    } else {
        this->logger->logInfo("PGConnection connected on {}:{}/{}", config.host, config.port, config.dbname);
        stats.connects.updated.store(CurrentTime<std::chrono::system_clock>::milliseconds(), std::memory_order_relaxed);
        return true;
    }
}

bool PGConnection::retryGuard(std::function<bool()> func) {
    for (unsigned int i = 0;; ++i) {
        if (func()) {
            return true;
        } else {
            std::this_thread::sleep_for(std::chrono::seconds{config.retryDelaySec});
            if (!resetConnect() && i == config.sendRetries) {
                return false;
            }
        }
    }
}

