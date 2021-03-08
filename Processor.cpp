//
// Created by imelker on 08.03.2021.
//

#include "Processor.h"

MessageProcessor::MessageProcessor() = default;
MessageProcessor::~MessageProcessor() = default;

void MessageProcessor::transformMessage(const IRCMessage &message, MessageData &result) {
    result.channel = message.parameters.at(0)[0] == '#' ? message.parameters.at(0).substr(1) : message.parameters.at(0);
    result.user = message.prefix.nickname;
    result.text = message.parameters.at(message.parameters.size() - 1);
    result.valid = true;
}

DataProcessor::DataProcessor() = default;
DataProcessor::~DataProcessor() = default;

void DataProcessor::processMessage(const MessageData &msg, PGconn *conn) {
    std::string request = fmt::format(
        "INSERT INTO \"user\" (id, name) VALUES (DEFAULT, '{}') ON CONFLICT DO NOTHING;"
        "INSERT INTO message (id, text, timestamp, channel_id, user_id) "
        "VALUES (DEFAULT,'{}', (SELECT TO_TIMESTAMP({}/1000.0)), "
                              "(SELECT id from channel WHERE name='{}' ), "
                              "(SELECT id FROM \"user\" WHERE name='{}') );",
        msg.user, msg.text, msg.timestamp, msg.channel, msg.user);

    PQsendQuery(conn, request.c_str());
    while (auto resp = PQgetResult(conn)) {
        if (PQresultStatus(resp) == PGRES_FATAL_ERROR)
            fprintf(stderr, "DB Error: %s%s\n", PQresultErrorMessage(resp), request.c_str());
        PQclear(resp);
    }
}
