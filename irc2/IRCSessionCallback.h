//
// Created by l2pic on 25.04.2021.
//

#ifndef IRCTEST__IRCSESSIONCALLBACK_H_
#define IRCTEST__IRCSESSIONCALLBACK_H_

#include <libircclient.h>

#include <string_view>
#include <vector>

class IRCSessionCallback : public irc_callbacks_t {
  public:
    IRCSessionCallback();
    virtual ~IRCSessionCallback() = default;

    // IRC socket level events
    virtual void onLog(const char* msg, int len) {};
    virtual void onConnected(std::string_view event, std::string_view host) {};
    virtual void onDisconnected(std::string_view event, std::string_view reason) {};
    virtual void onSendDataLen(int len) {};
    virtual void onRecvDataLen(int len) {};
    // IRC common events
    virtual void onLoggedIn(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    // IRC commands events
    virtual void onNick(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onQuit(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onJoin(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onPart(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onMode(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onUmode(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onTopic(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onKick(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onChannel(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onPrivmsg(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onNotice(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onChannelNotice(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onInvite(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onCtcpReq(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onCtcpRep(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onCtcpAction(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onUnknown(std::string_view event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onNumeric(unsigned int event, std::string_view origin, const std::vector<std::string_view>& params) {};
    virtual void onDccChatReq(std::string_view nick, std::string_view addr, unsigned int dccid) {};
    virtual void onDccSendReq(std::string_view nick, std::string_view addr, std::string_view filename, unsigned long size, unsigned int dccid) {};
};

#endif //IRCTEST__IRCSESSIONCALLBACK_H_
