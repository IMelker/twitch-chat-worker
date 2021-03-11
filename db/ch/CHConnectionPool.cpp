//
// Created by l2pic on 11.03.2021.
//

#include "CHConnectionPool.h"

CHConnectionPool::CHConnectionPool(CHConnectionConfig config, unsigned int count,
                                   std::shared_ptr<Logger> logger)
    : DBConnectionPool(count, std::move(logger)), config(std::move(config)) {
    init();
}


std::shared_ptr<DBConnection> CHConnectionPool::createConnection() {
    return std::make_shared<CHConnection>(this->config, this->logger);
}

