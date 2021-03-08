//
// Created by l2pic on 06.03.2021.
//

#include <algorithm>
#include <chrono>

#include <nlohmann/json.hpp>

#include "common/SysSignal.h"

#include "IRCtoDBConnector.h"

using json = nlohmann::json;

std::vector<std::string> getChannelsList(const std::shared_ptr<PGConnectionPool>& pgBackend) {
    auto conn = pgBackend->lockConnection();

    std::vector<std::string> channelsList;
    PQsendQuery(conn->connection(), "SELECT name FROM channel WHERE active IS TRUE;");
    while (auto resp = PQgetResult(conn->connection())) {
        if (PQresultStatus(resp) == PGRES_TUPLES_OK) {
            for (int i = 0; i < PQntuples(resp); ++i) {
                channelsList.emplace_back(PQgetvalue(resp, i, 0));
            }
        }

        if (PQresultStatus(resp) == PGRES_FATAL_ERROR)
            fprintf(stderr, "DB Error: %s\n", PQresultErrorMessage(resp));
        PQclear(resp);
    }

    pgBackend->unlockConnection(conn);

    return channelsList;
}

IRCtoDBConnector::IRCtoDBConnector(unsigned int threads) : pool(threads) {}
IRCtoDBConnector::~IRCtoDBConnector() = default;

void IRCtoDBConnector::initPGConnectionPool(PGConnectionConfig config, unsigned int connections) {
    assert(!this->pg);
    this->pg = std::make_shared<PGConnectionPool>(std::move(config), connections);

    // init db based data
    {
        auto channels = getChannelsList(this->pg);

        std::lock_guard lg(channelsMutex);
        this->watchChannels = std::move(channels);
    }
}

void IRCtoDBConnector::initIRCWorkers(const IRCConnectConfig& config, const std::string &credentials) {
    assert(workers.empty());

    std::ifstream credfile(credentials);
    json json = json::parse(credfile);

    for (auto &cred : json) {
        IRCClientConfig cfg;
        cfg.user = cred.at("user").get<std::string>();
        cfg.nick = cred.at("nick").get<std::string>();
        cfg.password = cred.at("password").get<std::string>();
        workers.emplace_back(config, std::move(cfg), this);
    }
}

void IRCtoDBConnector::onConnected(IRCWorker *worker) {
    printf("IRCClient onConnected\n");
}

void IRCtoDBConnector::onDisconnected(IRCWorker *worker) {
    printf("IRCClient onDisconnected\n");
}

void IRCtoDBConnector::onMessage(IRCMessage message, IRCWorker *worker) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    pool.enqueue([message = std::move(message), timestamp, this]() {
        MessageData transformed;
        transformed.timestamp = timestamp;
        this->msgProcessor.transformMessage(message, transformed);

        // process request to database
        if (transformed.valid) {
            pool.enqueue([message = std::move(transformed), this]() {
                auto conn = pg->lockConnection();
                dbProcessor.processMessage(message, conn->connection());
                pg->unlockConnection(conn);
            });
        }
    });
}

void IRCtoDBConnector::onLogin(IRCWorker *worker) {
    printf("IRCClient onLogin\n");

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

