//
// Created by l2pic on 07.03.2021.
//

#ifndef CHATSNIFFER_PG_PGCONNECTION_H_
#define CHATSNIFFER_PG_PGCONNECTION_H_

#include <memory>
#include <vector>
#include <atomic>
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
    struct {
        struct {
            std::atomic<long long> updated{};
            std::atomic<unsigned int> attempts{};
        } connects;
        struct {
            std::atomic<long long> updated{};
            std::atomic<unsigned int> rtt{};
            std::atomic<unsigned int> count{};
            std::atomic<unsigned int> failed{};
        } requests;
    } stats;
  public:
    explicit PGConnection(PGConnectionConfig  config, std::shared_ptr<Logger> logger);
    ~PGConnection() override = default;

    [[nodiscard]] PGconn *raw() const { return conn.get();}

    bool resetConnect();

    bool request(const std::string& request);
    bool request(const std::string& request, std::vector<std::vector<std::string>>& result);
    bool ping();

    [[nodiscard]] const decltype(stats)& getStats() const { return stats; };
  private:
    bool retryGuard(std::function<bool()> func);

    PGConnectionConfig config;
    std::shared_ptr<PGconn> conn;
};

#endif //CHATSNIFFER_PG_PGCONNECTION_H_
