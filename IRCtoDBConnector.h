//
// Created by l2pic on 06.03.2021.
//

#ifndef CHATSNIFFER__IRCTODBCONNECTOR_H_
#define CHATSNIFFER__IRCTODBCONNECTOR_H_

#include <vector>
#include <string>

#include "common/ThreadPool.h"

#include "pgsql/PGConnectionPool.h"
#include "ircclient/IRCWorker.h"

#include "Processor.h"

class IRCtoDBConnector : public IRCWorkerListener
{
  public:
    explicit IRCtoDBConnector(unsigned int threads);
    ~IRCtoDBConnector();

    void initPGConnectionPool(PGConnectionConfig config, unsigned int connections);
    void initIRCWorkers(const IRCConnectConfig& config, const std::string& credentials);

    static void loop();

    // implement IRCWorkerListener
    void onConnected(IRCWorker *worker) override;
    void onDisconnected(IRCWorker *worker) override;
    void onMessage(IRCMessage message, IRCWorker *worker) override;
    void onLogin(IRCWorker *worker) override;
  private:
    std::shared_ptr<PGConnectionPool> pg;
    std::vector<IRCWorker> workers;

    std::mutex channelsMutex;
    std::vector<std::string> watchChannels;

    MessageProcessor msgProcessor;
    DataProcessor dbProcessor;

    ThreadPool pool;
};

#endif //CHATSNIFFER__IRCTODBCONNECTOR_H_
