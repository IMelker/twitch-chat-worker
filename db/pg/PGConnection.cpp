//
// Created by imelker on 07.03.2021.
//

#include "../../common/Logger.h"
#include "PGConnection.h"

#include <utility>
#include <thread>
#include <chrono>

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

/*inline void sqlRequest(const std::string& request, const std::shared_ptr<PGConnectionPool> &pg) {
    auto &logger = pg->getLogger();
    logger->logTrace("PGConnection request: \"{}\"", request);
    {
        DBConnectionLock pgl(pg);
        PQsendQuery(pgl->raw(), request.c_str());
        while (auto resp = PQgetResult(pgl->raw())) {
            if (PQresultStatus(resp) == PGRES_FATAL_ERROR)
                logger->logError("PGConnection {}", PQresultErrorMessage(resp));
            PQclear(resp);
        }
    }
}*/

bool PGConnection::ping() {
    return retryGuard([this]() {
        bool success = false;
        PQsendQuery(raw(), "SELECT 1;");
        while (auto resp = PQgetResult(raw())) {
            auto status = PQresultStatus(resp);
            if (status == PGRES_TUPLES_OK) {
                success = true;
            }
            if (status == PGRES_FATAL_ERROR) {
                logger->logError("PGConnection Error: {}\n", PQresultErrorMessage(resp));
                success = false;
            }
            PQclear(resp);
        }
        return success;
    });
}

bool PGConnection::resetConnect() {
    conn.reset(PQsetdbLogin(config.host.c_str(), std::to_string(config.port).c_str(), nullptr, nullptr,
                            config.dbname.c_str(), config.user.c_str(), config.pass.c_str()), &PQfinish);

    if (PQstatus(conn.get()) != CONNECTION_OK && PQsetnonblocking(conn.get(), 1) != 0) {
        this->logger->logCritical("PGConnection {}:{} {}", config.host, config.port, PQerrorMessage(conn.get()));
        return false;
    } else {
        this->logger->logInfo("PGConnection connected on {}:{}/{}", config.host, config.port, config.dbname);
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

