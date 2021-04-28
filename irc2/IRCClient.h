//
// Created by l2pic on 25.04.2021.
//

#ifndef CHATCONTROLLER_IRC2_IRCCLIENT_H_
#define CHATCONTROLLER_IRC2_IRCCLIENT_H_

#include <libircclient.h>

#include <map>
#include <memory>

#include "IRCConnectionConfig.h"
#include "IRCClientConfig.h"
#include "IRCSession.h"

class IRCSelectorPool;
class IRCClient : public IRCSessionInterface
{
  public:
    IRCClient(IRCConnectionConfig conConfig, IRCClientConfig cliConfig, IRCSelectorPool *pool);;
    ~IRCClient();

    void createSession();

    void joinChannel();
    void joinChannels();

    // IRCSessionInterface implementation
    bool sendQuit(const std::string &reason) override;
    bool sendJoin(const std::string &channel) override;
    bool sendJoin(const std::string &channel, const std::string &key) override;
    bool sendPart(const std::string &channel) override;
    bool sendTopic(const std::string &channel, const std::string &topic) override;
    bool sendNames(const std::string &channel) override;
    bool sendList(const std::string &channel) override;
    bool sendInvite(const std::string &channel, const std::string &nick) override;
    bool sendKick(const std::string &channel, const std::string &nick, const std::string &comment) override;
    bool sendMessage(const std::string &channel, const std::string &text) override;
    bool sendNotice(const std::string &channel, const std::string &text) override;
    bool sendMe(const std::string &channel, const std::string &text) override;
    bool sendChannelMode(const std::string &channel, const std::string &mode) override;
    bool sendCtcpRequest(const std::string &nick, const std::string &reply) override;
    bool sendCtcpReply(const std::string &nick, const std::string &reply) override;
    bool sendUserMode(const std::string &mode) override;
    bool sendNick(const std::string &newnick) override;
    bool sendWhois(const std::string &nick) override;
    bool sendRaw(const std::string &raw) override;

  private:
    IRCConnectionConfig conConfig;
    IRCClientConfig cliConfig;

    IRCSelectorPool *pool;

    std::vector<std::shared_ptr<IRCSession>> sessions;
};

#endif //CHATCONTROLLER_IRC2_IRCCLIENT_H_
