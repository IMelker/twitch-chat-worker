//
// Created by imelker on 10.03.2021.
//

#ifndef CHATSNIFFER_PGSQL_PGCONNECTIONLOCKER_H_
#define CHATSNIFFER_PGSQL_PGCONNECTIONLOCKER_H_

#include <memory>
#include "DBConnectionPool.h"


template <typename Pool>
class DBConnectionLock
{
    using Connection = typename Pool::conn_type;
  public:
    explicit DBConnectionLock(const std::shared_ptr<Pool>& pool)
        : pool(pool), conn(std::static_pointer_cast<Connection>(pool->lockConnection())) {
    };
    ~DBConnectionLock() {
        if (conn)
            pool->unlockConnection(conn);
    };

    Connection *operator->() const { return conn.get(); }

  private:
    std::shared_ptr<Pool> pool;
    std::shared_ptr<Connection> conn;
};

#endif //CHATSNIFFER_PGSQL_PGCONNECTIONLOCKER_H_
