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
    virtual void onLog(const char* msg [[maybe_unused]], int len [[maybe_unused]]) {};
    virtual void onConnected(std::string_view event [[maybe_unused]],
                             std::string_view host [[maybe_unused]]) {};
    virtual void onDisconnected(std::string_view event [[maybe_unused]],
                                std::string_view reason [[maybe_unused]]) {};
    virtual void onSendDataLen(int len [[maybe_unused]]) {};
    virtual void onRecvDataLen(int len [[maybe_unused]]) {};
    // IRC common events
    virtual void onLoggedIn(std::string_view event [[maybe_unused]], std::string_view origin  [[maybe_unused]],
                            const std::vector<std::string_view>& params [[maybe_unused]]) {};
    // IRC commands events
    virtual void onNick(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                        const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onQuit(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                        const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onJoin(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                        const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onPart(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                        const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onMode(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                        const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onUmode(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                         const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onTopic(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                         const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onKick(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                        const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onChannel(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                           const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onPrivmsg(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                           const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onNotice(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                          const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onChannelNotice(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                                 const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onInvite(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                          const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onCtcpReq(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                           const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onCtcpRep(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                           const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onCtcpAction(std::string_view event [[maybe_unused]], std::string_view origin [[maybe_unused]],
                              const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onPong(std::string_view event [[maybe_unused]], std::string_view host [[maybe_unused]]) {};
    virtual void onUnknown(std::string_view event [[maybe_unused]], std::string_view origin  [[maybe_unused]],
                           const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onNumeric(unsigned int event [[maybe_unused]], std::string_view origin  [[maybe_unused]],
                           const std::vector<std::string_view>& params [[maybe_unused]]) {};
    virtual void onDccChatReq(std::string_view nick [[maybe_unused]], std::string_view addr [[maybe_unused]],
                              unsigned int dccid [[maybe_unused]]) {};
    virtual void onDccSendReq(std::string_view nick [[maybe_unused]], std::string_view addr [[maybe_unused]],
                              std::string_view filename [[maybe_unused]], unsigned long size [[maybe_unused]],
                              unsigned int dccid [[maybe_unused]]) {};
};

#endif //IRCTEST__IRCSESSIONCALLBACK_H_
