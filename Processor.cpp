//
// Created by imelker on 08.03.2021.
//

#include <libpq-fe.h>
#include <fmt/format.h>

#include "common/Utils.h"
#include "common/Logger.h"
#include "pgsql/PGConnection.h"

#include "Processor.h"

MessageProcessor::MessageProcessor() = default;
MessageProcessor::~MessageProcessor() = default;

static const std::string removeSymbol{"\'\"`<>~()[]{}#$%^&*"};

void MessageProcessor::transformMessage(const IRCMessage &message, MessageData &result) {
    result.channel = message.parameters.at(0)[0] == '#' ? message.parameters.at(0).substr(1) : message.parameters.at(0);
    result.user = message.prefix.nickname;

    result.text.resize(message.parameters.back().size(), '\0');
    int pos = 0;
    for (char c : message.parameters.back()) {
        if(removeSymbol.find(c) != std::string::npos)
            continue;
        result.text[pos++] = c;
    }
    result.text.resize(pos);

    result.valid = !result.text.empty();
}

DataProcessor::DataProcessor() = default;
DataProcessor::~DataProcessor() = default;

void DataProcessor::processMessage(const MessageData &msg, PGConnection *conn) {
    auto &logger = conn->getLogger();
    std::string request = fmt::format(
        "INSERT INTO \"user\" (id, name) VALUES (DEFAULT, '{}') ON CONFLICT DO NOTHING;"
        "INSERT INTO message (id, text, timestamp, channel_id, user_id) "
        "VALUES (DEFAULT, '{}', (SELECT TO_TIMESTAMP({}/1000.0)),"
                              " (SELECT id from channel WHERE name = '{}'), "
                              " (SELECT id FROM \"user\" WHERE name = '{}') );",
        msg.user, msg.text, msg.timestamp, msg.channel, msg.user);

    logger->logTrace("PGConnection request: \"{}\"", request);

    PQsendQuery(conn->connection(), request.c_str());
    while (auto resp = PQgetResult(conn->connection())) {
        if (PQresultStatus(resp) == PGRES_FATAL_ERROR)
            logger->logError("PGConnection {}", PQresultErrorMessage(resp));
        PQclear(resp);
    }
}
