//
// Created by imelker on 11.03.2021.
//

#ifndef CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_
#define CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_

#include <vector>
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

    // http request handlers
    std::vector<CHConnection::CHStatistics> collectStats() const;
  private:
    CHConnectionConfig config;
    std::vector<std::shared_ptr<CHConnection>> all;
};

#endif //CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_
