//
// Created by imelker on 11.03.2021.
//

#ifndef CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_
#define CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_

#include <vector>
#include "CHConnection.h"

#include "../DBConnectionPool.h"

#include "../../http/server/HTTPServerUnit.h"

class Logger;
class CHConnectionPool : public DBConnectionPool, public HTTPServerUnit
{
  public:
    using conn_type = CHConnection;
  public:
    CHConnectionPool(CHConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger);
    ~CHConnectionPool() override = default;

    std::shared_ptr<DBConnection> createConnection() override;

    [[nodiscard]] const std::vector<std::shared_ptr<CHConnection>>& getAll() const { return all; };

    // implement HTTPControlUnit
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &request, std::string &error) override;

    // request handlers
    std::string handleStats(const std::string &request, std::string &error);
  private:
    CHConnectionConfig config;
    std::vector<std::shared_ptr<CHConnection>> all;
};

#endif //CHATSNIFFER_DB_CH_CHCONNECTIONPOOL_H_
