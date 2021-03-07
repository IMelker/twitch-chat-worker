//
// Created by l2pic on 07.03.2021.
//

#ifndef CHATSNIFFER_PG_PGCONNECTION_H_
#define CHATSNIFFER_PG_PGCONNECTION_H_

#include <memory>
#include <mutex>
#include <libpq-fe.h>

struct PGConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string dbname = "demo";
    std::string user = "postgres";
    std::string pass = "postgres";
};

class PGConnection
{
  public:
    explicit PGConnection(const PGConnectionConfig& config = {});
    ~PGConnection() = default;

    [[nodiscard]] PGconn *connection() const { return conn.get();}
    [[nodiscard]] bool connected() const { return established; }
  private:
    bool established = false;
    std::shared_ptr<PGconn> conn;
};

#endif //CHATSNIFFER_PG_PGCONNECTION_H_
