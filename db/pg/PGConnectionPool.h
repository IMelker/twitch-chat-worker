//
// Created by imelker on 07.03.2021.
//

#ifndef CHATSNIFFER_PG_PGCONNECTIONPOOL_H_
#define CHATSNIFFER_PG_PGCONNECTIONPOOL_H_

#include <vector>
#include "PGConnection.h"

#include "../DBConnectionPool.h"
#include "../../http/server/HTTPServerUnit.h"

class Logger;
class PGConnectionPool : public DBConnectionPool, public HTTPServerUnit
{
  public:
    using conn_type = PGConnection;
  public:
    explicit PGConnectionPool(PGConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger);
    ~PGConnectionPool() override = default;

    std::shared_ptr<DBConnection> createConnection() override;

    // implement HTTPControlUnit
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &request, std::string &error) override;

    // request handlers
    std::string handleStats(const std::string &request, std::string &error);
  private:
    PGConnectionConfig config;
    std::vector<std::shared_ptr<PGConnection>> all;
};

#endif //CHATSNIFFER_PG_PGCONNECTIONPOOL_H_
