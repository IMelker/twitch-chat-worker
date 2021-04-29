//
// Created by l2pic on 25.04.2021.
//

#include <cassert>
#include <utility>

#include "IRCSelectorPool.h"
#include "IRCClient.h"

#define MIN_SESS_COUNT 1

IRCClient::IRCClient(IRCConnectionConfig conConfig, IRCClientConfig cliConfig, IRCSelectorPool *pool)
    : conConfig(std::move(conConfig)), cliConfig(std::move(cliConfig)), pool(pool) {
    assert(pool);
    for (int i = 0; i < std::max(MIN_SESS_COUNT, this->cliConfig.session_count); ++i) {
        createSession();
    }
}

IRCClient::~IRCClient() {
    for (auto &session: sessions) {
        pool->removeSession(session);

        session->disconnect();
    }
}

void IRCClient::createSession() {
    auto session = std::make_shared<IRCSession>(conConfig, cliConfig, this);
    if (!session->connect())
        return;

    sessions.push_back(session);
    pool->addSession(session);
}

void IRCClient::joinChannel(const std::string &channel) {
    sessions[curSessionRoundRobin++ % sessions.size()]->sendJoin(channel);
}

void IRCClient::joinChannels(const std::vector<std::string> &channels) {
    for (auto &channel: channels)
        sessions[curSessionRoundRobin++ % sessions.size()]->sendJoin(channel);
}

std::string IRCClient::gatherStatsAndDump() {
    IRCStatistic stats;
    for(auto &session: sessions)
        stats += session->statistic();
    return stats.dump();
}

bool IRCClient::sendQuit(const std::string &reason) {
    for(auto &session: sessions) {
        session->sendQuit(reason);
    }
    return true;
}

bool IRCClient::sendJoin(const std::string &channel) {
    return true;
}

bool IRCClient::sendJoin(const std::string &channel, const std::string &key) {
    return false;
}

bool IRCClient::sendPart(const std::string &channel) {
    return false;
}

bool IRCClient::sendTopic(const std::string &channel, const std::string &topic) {
    return false;
}

bool IRCClient::sendNames(const std::string &channel) {
    return false;
}

bool IRCClient::sendList(const std::string &channel) {
    return false;
}

bool IRCClient::sendInvite(const std::string &channel, const std::string &nick) {
    return false;
}

bool IRCClient::sendKick(const std::string &channel, const std::string &nick, const std::string &comment) {
    return false;
}

bool IRCClient::sendMessage(const std::string &channel, const std::string &text) {
    return false;
}

bool IRCClient::sendNotice(const std::string &channel, const std::string &text) {
    return false;
}

bool IRCClient::sendMe(const std::string &channel, const std::string &text) {
    return false;
}

bool IRCClient::sendChannelMode(const std::string &channel, const std::string &mode) {
    return false;
}

bool IRCClient::sendCtcpRequest(const std::string &nick, const std::string &reply) {
    return false;
}

bool IRCClient::sendCtcpReply(const std::string &nick, const std::string &reply) {
    return false;
}

bool IRCClient::sendUserMode(const std::string &mode) {
    return false;
}

bool IRCClient::sendNick(const std::string &newnick) {
    return false;
}

bool IRCClient::sendWhois(const std::string &nick) {
    return false;
}

bool IRCClient::sendPing(const std::string &host) {
    for(auto &session: sessions) {
        session->sendPing(host);
    }
    return true;
}

bool IRCClient::sendRaw(const std::string &raw) {
    return false;
}


