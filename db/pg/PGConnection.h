//
// Created by l2pic on 07.03.2021.
//

#ifndef CHATSNIFFER_PG_PGCONNECTION_H_
#define CHATSNIFFER_PG_PGCONNECTION_H_

#include <memory>
#include <functional>
#include <libpq-fe.h>

#include "../DBConnection.h"

struct PGConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string dbname = "demo";
    std::string user = "postgres";
    std::string pass = "postgres";
    unsigned int sendRetries = 5;
    unsigned int retryDelaySec = 5;
};

class Logger;
class PGConnection : public DBConnection
{
  public:
    explicit PGConnection(PGConnectionConfig  config, std::shared_ptr<Logger> logger);
    ~PGConnection() override = default;

    [[nodiscard]] PGconn *raw() const { return conn.get();}

    bool resetConnect();

    bool ping();
  private:
    bool retryGuard(std::function<bool()> func);

    PGConnectionConfig config;
    std::shared_ptr<PGconn> conn;
};

#endif //CHATSNIFFER_PG_PGCONNECTION_H_
