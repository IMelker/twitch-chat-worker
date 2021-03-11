//
// Created by imelker on 08.03.2021.
//

#include <cstring>
#include <libpq-fe.h>
#include <fmt/format.h>

#include "common/Utils.h"
#include "common/Logger.h"
#include "db/pg/PGConnectionPool.h"
#include "db/DBConnectionLock.h"

#include "Processor.h"

#define MESSAGE_BATCH_SIZE 1000

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

static const std::string kInsertUserFmt = "INSERT INTO \"user\" (id, name) VALUES (DEFAULT, '{}') ON CONFLICT DO NOTHING;";
static const std::string kInsertValueFmt = "(DEFAULT,'{}',(SELECT TO_TIMESTAMP({}/1000.0)),"
                                           "(SELECT id FROM channel WHERE name = '{}'),"
                                           "(SELECT id FROM \"user\" WHERE name = '{}')){}";

inline void sqlRequest(const std::string& request, const std::shared_ptr<PGConnectionPool> &pg) {
    auto &logger = pg->getLogger();
    logger->logTrace("PGConnection request: \"{}\"", request);
    {
        DBConnectionLock pgl(pg);
        PQsendQuery(pgl->raw(), request.c_str());
        while (auto resp = PQgetResult(pgl->raw())) {
            if (PQresultStatus(resp) == PGRES_FATAL_ERROR)
                logger->logError("PGConnection {}", PQresultErrorMessage(resp));
            PQclear(resp);
        }
    }
}

inline void insertUsers(const std::set<std::string>& users, const std::shared_ptr<PGConnectionPool> &pg) {
    std::string request;
    for (auto &user: users) {
        request.reserve(request.size() + kInsertUserFmt.size() + user.size());
        request.append(fmt::format(kInsertUserFmt, user));
    }

    sqlRequest(request, pg);
}

inline void insertMessages(const std::vector<MessageData>& messages, const std::shared_ptr<PGConnectionPool> &pg) {
    std::string request;
    request.append("INSERT INTO message (id, text, timestamp, channel_id, user_id) VALUES ");
    for (size_t i = 0; i < messages.size(); ++i) {
        auto &message = messages[i];
        request.reserve(request.size() + message.text.size() + message.channel.size() + message.user.size());
        request.append(fmt::format(kInsertValueFmt, message.text, message.timestamp, message.channel, message.user,
                                   ((i != messages.size() - 1) ? "," : ";")));
    }

    sqlRequest(request, pg);
}

DataProcessor::DataProcessor() {
    batch.reserve(MESSAGE_BATCH_SIZE);
}

DataProcessor::~DataProcessor() = default;

void DataProcessor::processMessage(MessageData &&msg, const std::shared_ptr<PGConnectionPool> &pg) {
    thread_local std::vector<MessageData> tempMessages;
    thread_local std::set<std::string> tempUsers;
    if (std::lock_guard lg(mutex); batch.size() < MESSAGE_BATCH_SIZE) {
        users.insert(msg.user);
        batch.push_back(std::move(msg));
        return;
    } else {
        tempUsers.swap(users);
        tempMessages.reserve(MESSAGE_BATCH_SIZE);
        tempMessages.swap(batch);
    }

    insertUsers(tempUsers, pg);
    tempUsers.clear();

    insertMessages(tempMessages, pg);
    tempMessages.clear();
}

void DataProcessor::flushMessages(const std::shared_ptr<PGConnectionPool> &pg) {
    std::lock_guard lg(mutex);

    insertUsers(users, pg);
    users.clear();

    insertMessages(batch, pg);
    batch.clear();
}
