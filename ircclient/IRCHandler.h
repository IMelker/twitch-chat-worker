#ifndef _IRCHANDLER_H
#define _IRCHANDLER_H

#include "IRCClient.h"

#define NUM_IRC_CMDS 26

struct IRCCommandHandler
{
    std::string command;
    void (IRCClient::*handler)(IRCMessage message);
};

extern IRCCommandHandler ircCommandTable[NUM_IRC_CMDS];

inline int getCommandHandler(const std::string& command) {
    for (int i = 0; i < NUM_IRC_CMDS; ++i) {
        if (ircCommandTable[i].command == command)
            return i;
    }

    return NUM_IRC_CMDS;
}

#endif
