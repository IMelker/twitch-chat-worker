//
// Created by l2pic on 12.03.2021.
//

#ifndef CHATSNIFFER_DB_DBCONNECTION_H_
#define CHATSNIFFER_DB_DBCONNECTION_H_

#include <memory>

class Logger;
class DBConnection
{
  public:
    explicit DBConnection(std::shared_ptr<Logger> logger);
    virtual ~DBConnection();

    [[nodiscard]] bool connected() const { return established; }

    [[nodiscard]] const std::shared_ptr<Logger>& getLogger() const { return logger; }

  protected:
    bool established = false;
    std::shared_ptr<Logger> logger;
};

#endif //CHATSNIFFER_DB_DBCONNECTION_H_
