//
// Created by l2pic on 04.05.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCSESSIONINTERFACE_H_
#define CHATCONTROLLER_IRC_IRCSESSIONINTERFACE_H_

#include <string>

struct IRCSessionInterface {
    virtual bool sendQuit(const std::string& reason) = 0;
    virtual bool sendJoin(const std::string& channel) = 0;
    virtual bool sendJoin(const std::string& channel, const std::string& key) = 0;
    virtual bool sendPart(const std::string& channel) = 0;
    virtual bool sendTopic(const std::string& channel, const std::string& topic) = 0;
    virtual bool sendNames(const std::string& channel) = 0;
    virtual bool sendList(const std::string& channel) = 0;
    virtual bool sendInvite(const std::string &channel, const std::string &nick) = 0;
    virtual bool sendKick(const std::string &channel, const std::string &nick, const std::string &comment) = 0;
    virtual bool sendMessage(const std::string& channel, const std::string& text) = 0;
    virtual bool sendNotice(const std::string& channel, const std::string& text) = 0;
    virtual bool sendMe(const std::string& channel, const std::string& text) = 0;
    virtual bool sendChannelMode(const std::string& channel, const std::string& mode) = 0;
    virtual bool sendCtcpRequest(const std::string& nick, const std::string& reply) = 0;
    virtual bool sendCtcpReply(const std::string& nick, const std::string& reply) = 0;
    virtual bool sendUserMode(const std::string& mode) = 0;
    virtual bool sendNick(const std::string& newnick) = 0;
    virtual bool sendWhois(const std::string& nick) = 0;
    virtual bool sendPing(const std::string &host) = 0;
    virtual bool sendRaw(const std::string& raw) = 0;
};

#endif //CHATCONTROLLER_IRC_IRCSESSIONINTERFACE_H_
