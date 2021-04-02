//
// Created by l2pic on 03.04.2021.
//

#ifndef CHATSNIFFER__DBSTORAGE_H_
#define CHATSNIFFER__DBSTORAGE_H_

#include <memory>
#include "db/pg/PGConnectionPool.h"

class Logger;
class IRCClientConfig;
class DBStorage
{
  public:
    DBStorage(PGConnectionConfig config, unsigned int count, std::shared_ptr<Logger> logger);
    ~DBStorage();

    [[nodiscard]] PGConnectionPool *getPGPool() const;

    std::vector<IRCClientConfig> loadAccounts();
    std::vector<std::string> loadChannels();
  private:
    std::shared_ptr<Logger> logger;
    std::shared_ptr<PGConnectionPool> pg;
};

#endif //CHATSNIFFER__DBSTORAGE_H_
