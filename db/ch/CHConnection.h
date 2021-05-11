//
// Created by imelker on 11.03.2021.
//

#ifndef CHATSNIFFER_DB_CH_CHCONNECTION_H_
#define CHATSNIFFER_DB_CH_CHCONNECTION_H_

#include <memory>
#include <mutex>
#include <algorithm>

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
  public:
    struct CHStatistics {
        long long rtt = 0;
        unsigned int count = 0;
        unsigned int failed = 0;

        inline CHStatistics& operator+(const CHStatistics& rhs) noexcept {
            count += rhs.count;
            failed += rhs.failed;
            if (rhs.rtt > 0)
                rtt = rhs.rtt;
            return *this;
        }
        inline CHStatistics& operator+=(const CHStatistics & rhs) {
            return operator+(rhs);
        }
    };
  public:
    CHConnection(const CHConnectionConfig& config, std::shared_ptr<Logger> logger);
    ~CHConnection() override = default;

    [[nodiscard]] clickhouse::Client *raw() const { return conn.get();}

    void insert(const std::string& table_name, const clickhouse::Block& block) {
        auto first = CurrentTime<std::chrono::system_clock>::milliseconds();
        try {
            conn->Insert(table_name, block);
            auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
            {
                std::lock_guard lg(statsMutex);
                ++stats.count;
                stats.rtt = now - first;
            }
        } catch (const std::exception& e) {
            fprintf(stderr, "Failed to insert to CH: %s\n", e.what());
            auto now = CurrentTime<std::chrono::system_clock>::milliseconds();
            {
                std::lock_guard lg(statsMutex);
                ++stats.failed;
                stats.rtt = now - first;
            }
        }
    }

    [[nodiscard]] CHStatistics getStats() {
        CHStatistics temp;
        {
            std::lock_guard lg(statsMutex);
            std::swap(temp, stats);
        }
        return temp;
    };
  private:
    std::shared_ptr<clickhouse::Client> conn;

    std::mutex statsMutex;
    CHStatistics stats;
};

#endif //CHATSNIFFER_DB_CH_CHCONNECTION_H_
