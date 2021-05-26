//
// Created by l2pic on 04.05.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCSESSIONLISTENER_H_
#define CHATCONTROLLER_IRC_IRCSESSIONLISTENER_H_

struct IRCMessage;
struct IRCStatistic;
class IRCSession;
struct IRCSessionListener {
    virtual void onLoggedIn(IRCSession* session) = 0;
    virtual void onDisconnected(IRCSession *session, std::string_view reason) = 0;
    virtual void onStatistics(IRCSession* session, IRCStatistic&& stats) = 0;
    virtual void onMessage(IRCMessage &&message) = 0;
};

#endif //CHATCONTROLLER_IRC_IRCSESSIONLISTENER_H_
