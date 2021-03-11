//
// Created by imelker on 11.03.2021.
//

#ifndef CHATSNIFFER_DB_CH_CHCONNECTION_H_
#define CHATSNIFFER_DB_CH_CHCONNECTION_H_

#include <memory>

#include <clickhouse/client.h>

#include "../DBConnection.h"

struct CHConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string dbname = "demo";
    std::string user = "postgres";
    std::string pass = "postgres";
};

class Logger;
class CHConnection : public DBConnection
{
  public:
    CHConnection(const CHConnectionConfig& config, std::shared_ptr<Logger> logger);
    ~CHConnection() override = default;

    [[nodiscard]] clickhouse::Client *raw() const { return conn.get();}

  private:
    std::shared_ptr<clickhouse::Client> conn;
};

#endif //CHATSNIFFER_DB_CH_CHCONNECTION_H_
