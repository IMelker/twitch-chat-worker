//
// Created by l2pic on 06.03.2021.
//

#include <algorithm>
#include <chrono>

#include <nlohmann/json.hpp>

#include "common/SysSignal.h"
#include "common/Logger.h"

#include "db/DBConnectionLock.h"
#include "IRCtoDBConnector.h"

using json = nlohmann::json;

std::vector<std::string> getChannelsList(const std::shared_ptr<PGConnectionPool>& pgBackend) {
    auto &logger = pgBackend->getLogger();

    std::string request = "SELECT name FROM channel WHERE watch IS TRUE;";
    logger->logTrace("PGConnection request: \"{}\"", request);

    std::vector<std::string> channelsList;
    {
        DBConnectionLock dbl(pgBackend);
        PQsendQuery(dbl->raw(), request.c_str());
        while (auto resp = PQgetResult(dbl->raw())) {
            if (PQresultStatus(resp) == PGRES_TUPLES_OK) {
                for (int i = 0; i < PQntuples(resp); ++i) {
                    channelsList.emplace_back(PQgetvalue(resp, i, 0));
                }
            }

            if (PQresultStatus(resp) == PGRES_FATAL_ERROR) {
                logger->logError("DBConnection Error: {}\n", PQresultErrorMessage(resp));
            }
            PQclear(resp);
        }
    }

    return channelsList;
}

IRCtoDBConnector::IRCtoDBConnector(unsigned int threads, std::shared_ptr<Logger> logger)
    : logger(std::move(logger)), pool(threads) {
    this->logger->logInfo("IRCtoDBConnector created with {} threads", threads);
}

IRCtoDBConnector::~IRCtoDBConnector() {
    dbProcessor.flushMessages(ch);
    logger->logTrace("IRCtoDBConnector end of connector");
};

void IRCtoDBConnector::initPGConnectionPool(PGConnectionConfig config, unsigned int connections, std::shared_ptr<Logger> logger) {
    assert(!this->pg);
    this->logger->logInfo("IRCtoDBConnector init PostgreSQL DB pool with {} connections", connections);
    pg = std::make_shared<PGConnectionPool>(std::move(config), connections, std::move(logger));

    // init db based data
    {
        auto channels = getChannelsList(pg);

        this->logger->logInfo("IRCtoDBConnector {} channels added to watch list", channels.size());

        std::lock_guard lg(channelsMutex);
        this->watchChannels = std::move(channels);
    }
}

void IRCtoDBConnector::initCHConnectionPool(CHConnectionConfig config, unsigned int connections, std::shared_ptr<Logger> logger) {
    assert(!this->ch);
    this->logger->logInfo("IRCtoDBConnector init Clickhouse DB pool with {} connections", connections);
    ch = std::make_shared<CHConnectionPool>(std::move(config), connections, std::move(logger));
}

void IRCtoDBConnector::initIRCWorkers(const IRCConnectConfig& config, const std::string &credentials, std::shared_ptr<Logger> logger) {
    assert(workers.empty());

    std::ifstream credfile(credentials);
    json json = json::parse(credfile);

    this->logger->logInfo("IRCtoDBConnector {} users loaded from {}", json.size(), credentials);

    for (auto &cred : json) {
        IRCClientConfig cfg;
        cfg.user = cred.at("user").get<std::string>();
        cfg.nick = cred.at("nick").get<std::string>();
        cfg.password = cred.at("password").get<std::string>();
        workers.emplace_back(config, std::move(cfg), this, logger);
    }
}

void IRCtoDBConnector::onConnected(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] connected", fmt::ptr(worker));
}

void IRCtoDBConnector::onDisconnected(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] disconnected", fmt::ptr(worker));
}

void IRCtoDBConnector::onMessage(IRCMessage message, IRCWorker *worker) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    pool.enqueue([message = std::move(message), timestamp, this]() {
        MessageData transformed;
        transformed.timestamp = timestamp;
        msgProcessor.transformMessage(message, transformed);

        logger->logTrace(R"(IRCtoDBConnector process {{channel: "{}",from: "{}", text: "{}", valid: {}}})",
                               transformed.channel, transformed.user, transformed.text, transformed.valid);

        // process request to database
        if (transformed.valid) {
            pool.enqueue([msg = std::move(transformed), this]() mutable {
                if (ch) {
                    dbProcessor.processMessage(std::move(msg), ch);
                }
            });
        }
    });
}

void IRCtoDBConnector::onLogin(IRCWorker *worker) {
    logger->logTrace("IRCtoDBConnector IRCWorker[{}] logged in", fmt::ptr(worker));

    std::lock_guard lg(channelsMutex);
    size_t pos = 0;
    for (pos = 0; pos < watchChannels.size(); ++pos) {
        if (!worker->joinChannel(watchChannels[pos]))
            break;
    }
    watchChannels.erase(watchChannels.begin(), watchChannels.begin() + pos);
}

void IRCtoDBConnector::loop() {
    while (!SysSignal::serviceTerminated()){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

