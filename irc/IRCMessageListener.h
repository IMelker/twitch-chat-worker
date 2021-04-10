//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER_IRC_IRCMESSAGELISTENER_H_
#define CHATSNIFFER_IRC_IRCMESSAGELISTENER_H_

class IRCWorker;
class IRCMessage;
struct IRCMessageListener {
    virtual void onMessage(IRCWorker *worker, const IRCMessage &message, long long now) {};
    virtual void onMessage(const IRCMessage &message) {};
};

#endif //CHATSNIFFER_IRC_IRCMESSAGELISTENER_H_