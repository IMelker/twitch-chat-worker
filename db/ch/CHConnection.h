//
// Created by imelker on 11.03.2021.
//

#ifndef CHATSNIFFER_DB_CH_CHCONNECTION_H_
#define CHATSNIFFER_DB_CH_CHCONNECTION_H_

#include <memory>
#include <atomic>

#include <clickhouse/client.h>

#include "../DBConnection.h"
#include "../../common/Clock.h"

struct CHConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string dbname = "demo";
    std::string user = "postgres";
    std::string pass = "postgres";
    bool secure = true;
    bool verify = true;
    unsigned int sendRetries = 5;
};

class Logger;
class CHConnection : public DBConnection
{
    struct {
        struct {
            std::atomic<long long> updated{};
            std::atomic<unsigned int> rtt{};
            std::atomic<unsigned int> count{};
            std::atomic<unsigned int> failed{};
        } requests;
    } stats;
  public:
    CHConnection(const CHConnectionConfig& config, std::shared_ptr<Logger> logger);
    ~CHConnection() override = default;

    [[nodiscard]] clickhouse::Client *raw() const { return conn.get();}

    void insert(const std::string& table_name, const clickhouse::Block& block) {
        auto first = CurrentTime<std::chrono::system_clock>::milliseconds();

        conn->Insert(table_name, block);

        auto now = CurrentTime<std::chrono::system_clock>::milliseconds();

        stats.requests.updated.store(now, std::memory_order_relaxed);
        stats.requests.count.fetch_add(1, std::memory_order_relaxed);
        stats.requests.rtt.store(now - first, std::memory_order_relaxed);
    }

    [[nodiscard]] const decltype(stats)& getStats() const { return stats; };
  private:
    std::shared_ptr<clickhouse::Client> conn;
};

#endif //CHATSNIFFER_DB_CH_CHCONNECTION_H_
