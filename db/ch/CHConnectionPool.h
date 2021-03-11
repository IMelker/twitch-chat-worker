//
// Created by l2pic on 11.03.2021.
//

#ifndef CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_
#define CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_

#include "CHConnection.h"

#include "../DBConnectionPool.h"

class Logger;
class CHConnectionPool : public DBConnectionPool
{
  public:
    using conn_type = CHConnection;
  public:
    CHConnectionPool(CHConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger);
    ~CHConnectionPool() override = default;

    std::shared_ptr<DBConnection> createConnection() override;
  private:
    CHConnectionConfig config;
};

#endif //CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_
