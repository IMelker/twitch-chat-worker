//
// Created by l2pic on 07.03.2021.
//

#ifndef CHATSNIFFER_PG_PGCONNECTIONPOOL_H_
#define CHATSNIFFER_PG_PGCONNECTIONPOOL_H_

#include "PGConnection.h"

#include "../DBConnectionPool.h"

class Logger;
class PGConnectionPool : public DBConnectionPool
{
  public:
    using conn_type = PGConnection;
  public:
    explicit PGConnectionPool(PGConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger);
    ~PGConnectionPool() override = default;

    std::shared_ptr<DBConnection> createConnection() override;
  private:
    PGConnectionConfig config;
};

#endif //CHATSNIFFER_PG_PGCONNECTIONPOOL_H_
