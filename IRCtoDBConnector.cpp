//
// Created by l2pic on 06.03.2021.
//

#include <algorithm>
#include <chrono>

#include "IRCtoDBConnector.h"

IRCtoDBConnector::IRCtoDBConnector(std::shared_ptr<PGConnectionPool> pg) : pg(std::move(pg)) {

}

IRCtoDBConnector::~IRCtoDBConnector() = default;

void IRCtoDBConnector::onConnected(IRCClient *client) {

}

void IRCtoDBConnector::onDisconnected(IRCClient *client) {

}

void insertMessageData(const std::string& channel, const std::string& from, const std::string& text,
                       long timestamp, const std::shared_ptr<PGConnectionPool>& pgBackend) {
    auto conn = pgBackend->lockConnection();

    std::string request = "INSERT INTO \"user\" (id, name) VALUES (DEFAULT, '" + from + "') ON CONFLICT DO NOTHING;";
    request.append("INSERT INTO message (id, text, timestamp, channel_id, user_id) VALUES (DEFAULT,'")
           .append(text).append("', (SELECT TO_TIMESTAMP(").append(std::to_string(timestamp)).append("/1000.0)), ")
           .append("(SELECT id from channel WHERE name='").append(channel.substr(1)).append("' ), ")
           .append("(SELECT id FROM \"user\" WHERE name = '").append(from).append("') );");

    PQsendQuery(conn->connection(), request.c_str());

    while (auto resp = PQgetResult(conn->connection())) {
        if (PQresultStatus(resp) == PGRES_FATAL_ERROR)
            fprintf(stderr, "DB Error: %s%s\n", PQresultErrorMessage(resp), request.c_str());
        PQclear(resp);
    }
    pgBackend->unlockConnection(conn);
}

void IRCtoDBConnector::onMessage(IRCMessage message, IRCClient *client) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    insertMessageData(message.parameters.at(0), message.prefix.nickname,
                      message.parameters.at(message.parameters.size() - 1), timestamp, pg);
}

void joinToChannels(const std::vector<std::string>& list, IRCClient *client) {
    if (!client || !client->connected())
        return;

    for(auto &name: list)
        client->sendIRC("JOIN #" + name + "\n");
}

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

void IRCtoDBConnector::onLogin(IRCClient *client) {
    //20 join attempts per 10 seconds per user (2000 for verified bots)
    joinToChannels(getChannelsList(pg), client);
}
