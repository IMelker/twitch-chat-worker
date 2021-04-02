//
// Created by l2pic on 02.04.2021.
//

#ifndef CHATSNIFFER_IRC_IRCMESSAGELISTENER_H_
#define CHATSNIFFER_IRC_IRCMESSAGELISTENER_H_

class IRCWorker;
class IRCMessage;
struct IRCMessageListener {
    virtual void onMessage(IRCWorker *worker, const IRCMessage &message, long long now) = 0;
};

#endif //CHATSNIFFER_IRC_IRCMESSAGELISTENER_H_
