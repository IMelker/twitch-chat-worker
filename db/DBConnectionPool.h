//
// Created by l2pic on 12.03.2021.
//

#ifndef CHATSNIFFER_DB_DBCONNECTIONPOOL_H_
#define CHATSNIFFER_DB_DBCONNECTIONPOOL_H_

#include <memory>
#include <mutex>
#include <string>
#include <queue>
#include <condition_variable>

class Logger;
class DBConnection;

class DBConnectionPool
{
  public:
    DBConnectionPool(unsigned int count, std::shared_ptr<Logger> logger);
    virtual ~DBConnectionPool();

    virtual std::shared_ptr<DBConnection> createConnection() = 0;

    std::shared_ptr<DBConnection> lockConnection();
    void unlockConnection(std::shared_ptr<DBConnection> conn);

    [[nodiscard]] size_t size() const { return pool.size(); }

    [[nodiscard]] const std::shared_ptr<Logger>& getLogger() const { return logger; };

  protected:
    void init();

    std::shared_ptr<Logger> logger;

  private:
    bool active = false;
    const unsigned int count = 10;

    std::mutex mutex;
    std::condition_variable condition;
    std::queue<std::shared_ptr<DBConnection>> pool;
};

#endif //CHATSNIFFER_DB_DBCONNECTIONPOOL_H_
