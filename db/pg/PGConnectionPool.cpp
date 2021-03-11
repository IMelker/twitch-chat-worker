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
    return std::make_shared<PGConnection>(this->config, this->logger);
}
