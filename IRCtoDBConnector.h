//
// Created by l2pic on 06.03.2021.
//

#ifndef CHATSNIFFER__IRCTODBCONNECTOR_H_
#define CHATSNIFFER__IRCTODBCONNECTOR_H_

#include <vector>
#include <string>

#include "pg/PGConnectionPool.h"
#include "ircclient/IRCWorker.h"

class IRCtoDBConnector : public IRCWorkerListener
{
  public:
    explicit IRCtoDBConnector(std::shared_ptr<PGConnectionPool> pg);
    ~IRCtoDBConnector();

    // implement IRCWorkerListener
    void onConnected(IRCClient *client) override;
    void onDisconnected(IRCClient *client) override;
    void onMessage(IRCMessage message, IRCClient *client) override;
    void onLogin(IRCClient *client) override;
  private:
    std::shared_ptr<PGConnectionPool> pg;
};

#endif //CHATSNIFFER__IRCTODBCONNECTOR_H_
