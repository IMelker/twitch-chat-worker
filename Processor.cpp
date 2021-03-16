//
// Created by imelker on 08.03.2021.
//

#include <cstring>
#include <libpq-fe.h>
#include <fmt/format.h>

#include "common/Utils.h"
#include "common/Logger.h"
#include "db/pg/PGConnectionPool.h"
#include "db/ch/CHConnectionPool.h"
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

//static const std::string kInsertUserFmt = "INSERT INTO \"user\" (id, name) VALUES (DEFAULT, '{}') ON CONFLICT DO NOTHING;";
//static const std::string kInsertValueFmt = "(DEFAULT,'{}',(SELECT TO_TIMESTAMP({}/1000.0)),"
//                                           "(SELECT id FROM channel WHERE name = '{}'),"
//                                           "(SELECT id FROM \"user\" WHERE name = '{}')){}";

/*inline void sqlRequest(const std::string& request, const std::shared_ptr<PGConnectionPool> &pg) {
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
}*/

inline void insertMessages(const std::vector<MessageData>& messages, const std::shared_ptr<CHConnectionPool> &ch) {
    using namespace clickhouse;

    auto channels = std::make_shared<ColumnFixedString>(256);
    auto user = std::make_shared<ColumnFixedString>(256);
    auto text = std::make_shared<ColumnString>();
    //auto tags = std::make_shared<ColumnString>();
    auto timestamps = std::make_shared<ColumnDateTime64>(3);
    for (const auto & message : messages) {
        channels->Append(message.channel);
        user->Append(message.user);
        text->Append(message.text);
        //tags->Append("{}");
        timestamps->Append(message.timestamp);
    }

    Block block;
    block.AppendColumn("channel", channels);
    block.AppendColumn("from", user);
    block.AppendColumn("text", text);
    //block.AppendColumn("tags", tags);
    block.AppendColumn("timestamp", timestamps);
    try {
        DBConnectionLock chl(ch);
        chl->raw()->Insert("twitch_chat.messages", block);
        ch->getLogger()->logInfo("Clickhouse insert {} messages", messages.size());
    } catch (const clickhouse::ServerException& err) {
        ch->getLogger()->logError("Clickhouse {}", err.what());
    }
}

DataProcessor::DataProcessor() {
    batch.reserve(MESSAGE_BATCH_SIZE);
}

DataProcessor::~DataProcessor() = default;


void DataProcessor::processMessage(MessageData &&msg, const std::shared_ptr<CHConnectionPool> &ch) {
    thread_local std::vector<MessageData> tempMessages;
    if (std::lock_guard lg(mutex); batch.size() < MESSAGE_BATCH_SIZE) {
        batch.push_back(std::move(msg));
        return;
    } else {
        tempMessages.reserve(MESSAGE_BATCH_SIZE);
        tempMessages.swap(batch);
    }

    insertMessages(tempMessages, ch);
    tempMessages.clear();
}

void DataProcessor::flushMessages(const std::shared_ptr<CHConnectionPool> &ch) {
    std::vector<MessageData> tempMessages;
    {
        std::lock_guard lg(mutex);
        tempMessages.swap(batch);
    }

    insertMessages(tempMessages, ch);

    ch->getLogger()->flush();
}
