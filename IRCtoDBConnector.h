//
// Created by l2pic on 06.03.2021.
//

#ifndef CHATSNIFFER__IRCTODBCONNECTOR_H_
#define CHATSNIFFER__IRCTODBCONNECTOR_H_

#include <map>
#include <string>

#include "common/ThreadPool.h"

#include "http/server/HTTPServer.h"
#include "ircclient/IRCWorker.h"
#include "db/pg/PGConnectionPool.h"
#include "db/ch/CHConnectionPool.h"

#include "Processor.h"

class Logger;
class IRCtoDBConnector : public IRCWorkerListener, public HTTPServerUnit
{
  public:
    explicit IRCtoDBConnector(unsigned int threads, std::shared_ptr<Logger> logger);
    ~IRCtoDBConnector();

    void initCHConnectionPool(CHConnectionConfig config, unsigned int connections, std::shared_ptr<Logger> logger);
    void initPGConnectionPool(PGConnectionConfig config, unsigned int connections, std::shared_ptr<Logger> logger);
    void initIRCWorkers(const IRCConnectConfig& config, std::vector<IRCClientConfig> accounts, std::shared_ptr<Logger> ircLogger);

    [[nodiscard]] ThreadPool * getPool();
    [[nodiscard]] const std::shared_ptr<PGConnectionPool> & getPG() const;
    [[nodiscard]] const std::shared_ptr<CHConnectionPool> & getCH() const;;

    static void loop();

    // controll data
    std::vector<IRCClientConfig> loadAccounts();
    std::vector<std::string> loadChannels();

    // irc settings
    void updateChannelsList(std::vector<std::string>&& channels);

    // implement IRCWorkerListener
    void onConnected(IRCWorker *worker) override;
    void onDisconnected(IRCWorker *worker) override;
    void onMessage(IRCWorker *worker, const IRCMessage &message, long long now) override;
    void onLogin(IRCWorker *worker) override;

    // implement HTTPControlUnit
    std::tuple<int, std::string> processHttpRequest(std::string_view path, const std::string &body, std::string &error) override;

    // request handlers
    std::string handleShutdown(const std::string &request, std::string &error);
    std::string handleVersion(const std::string &request, std::string &error);
    std::string handleAccounts(const std::string &request, std::string &error);
    std::string handleChannels(const std::string &request, std::string &error);
    std::string handleJoin(const std::string &request, std::string &error);
    std::string handleLeave(const std::string &request, std::string &error);
    std::string handleReloadChannels(const std::string &request, std::string &error);
    std::string handleMessage(const std::string &request, std::string &error);
    std::string handleCustom(const std::string &request, std::string &error);

  private:
    std::shared_ptr<Logger> logger;

    DataProcessor dbProcessor;
    MessageProcessor msgProcessor;

    std::shared_ptr<PGConnectionPool> pg;
    std::shared_ptr<CHConnectionPool> ch;

    ThreadPool pool;

    std::map<std::string, std::shared_ptr<IRCWorker>, std::less<>> workers;

    std::mutex watchMutex;
    std::map<std::string, IRCWorker*> watchChannels;
};

#endif //CHATSNIFFER__IRCTODBCONNECTOR_H_
