//
// Created by imelker on 07.03.2021.
//

#include "../common/Logger.h"
#include "PGConnection.h"

PGConnection::PGConnection(const PGConnectionConfig& config, std::shared_ptr<Logger> logger)
  : logger(std::move(logger)) {
    conn.reset(PQsetdbLogin(config.host.c_str(), std::to_string(config.port).c_str(), nullptr, nullptr,
                               config.dbname.c_str(), config.user.c_str(), config.pass.c_str()), &PQfinish);

    if (PQstatus(conn.get()) != CONNECTION_OK && PQsetnonblocking(conn.get(), 1) != 0) {
        this->logger->logCritical("PGConnection {}:{} {}", config.host, config.port, PQerrorMessage(conn.get()));
    } else {
        this->logger->logInfo("PGConnection connected on {}:{}/{}", config.host, config.port, config.dbname);
        established = true;
    }
}
