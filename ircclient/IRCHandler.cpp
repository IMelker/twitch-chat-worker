#include "IRCHandler.h"

IRCCommandHandler ircCommandTable[NUM_IRC_CMDS] =
{
    { "PRIVMSG", &IRCClient::handlePrivMsg      },
    { "NOTICE", &IRCClient::handleNotice        },
    { "JOIN", &IRCClient::handleChannelJoinPart },
    { "PART", &IRCClient::handleChannelJoinPart },
    { "NICK", &IRCClient::handleUserNickChange  },
    { "QUIT", &IRCClient::handleUserQuit        },
    { "353", &IRCClient::handleChannelNamesList },
    { "433", &IRCClient::handleNicknameInUse    },
    { "001", &IRCClient::handleServerMessage    },
    { "002", &IRCClient::handleServerMessage    },
    { "003", &IRCClient::handleServerMessage    },
    { "004", &IRCClient::handleServerMessage    },
    { "005", &IRCClient::handleServerMessage    },
    { "250", &IRCClient::handleServerMessage    },
    { "251", &IRCClient::handleServerMessage    },
    { "252", &IRCClient::handleServerMessage    },
    { "253", &IRCClient::handleServerMessage    },
    { "254", &IRCClient::handleServerMessage    },
    { "255", &IRCClient::handleServerMessage    },
    { "265", &IRCClient::handleServerMessage    },
    { "266", &IRCClient::handleServerMessage    },
    { "366", &IRCClient::handleServerMessage    },
    { "372", &IRCClient::handleServerMessage    },
    { "375", &IRCClient::handleServerMessage    },
    { "376", &IRCClient::handleServerMessage    },
    { "439", &IRCClient::handleServerMessage    }
};

void IRCClient::handleCTCP(IRCMessage message) {
    std::string to = message.parameters.at(0);
    std::string text = message.parameters.at(message.parameters.size() - 1);

    // Remove '\001' from start/end of the string
    text = text.substr(1, text.size() - 2);

    //printf("[%s requested CTCP %s]\n", message.prefix.nickname.c_str(), text.c_str());

    if (to == nick) {
        if (text == "VERSION") // Respond to CTCP VERSION
        {
            sendIRC("NOTICE " + message.prefix.nickname
                        + " :\001VERSION Open source IRC client by Fredi Machado - https://github.com/fredimachado/IRCClient \001\n");
            return;
        }

        // CTCP not implemented
        sendIRC("NOTICE " + message.prefix.nickname + " :\001ERRMSG " + text + " :Not implemented\001\n");
    }
}

void IRCClient::handlePrivMsg(IRCMessage message) {
    std::string to = message.parameters.at(0);
    std::string text = message.parameters.at(message.parameters.size() - 1);

    // Handle Client-To-Client Protocol
    if (text[0] == '\001') {
        handleCTCP(message);
        return;
    }

    if (to[0] == '#') {
        //printf("[%s] %s: %s\n", to.c_str(), message.prefix.nickname.c_str(), text.c_str());
    } else {
        //printf("< %s: %s\n", message.prefix.nickname.c_str(), text.c_str());
    }
}

void IRCClient::handleNotice(IRCMessage message) {
    std::string from = message.prefix.nickname.empty() ? message.prefix.prefix : message.prefix.nickname;
    std::string text;

    if (!message.parameters.empty())
        text = message.parameters.at(message.parameters.size() - 1);

    if (!text.empty() && text[0] == '\001') {
        text = text.substr(1, text.size() - 2);
        if (text.find(' ') == std::string::npos) {
            fprintf(stderr, "[Invalid %s reply from %s]\n", text.c_str(), from.c_str());
            return;
        }
        std::string ctcp = text.substr(0, text.find(' '));
        //printf("[%s %s reply]: %s\n", from.c_str(), ctcp.c_str(), text.substr(text.find(' ') + 1).c_str());
    } else{
        //printf("-%s- %s\n", from.c_str(), text.c_str());
    }
}

void IRCClient::handleChannelJoinPart(IRCMessage message) {
    std::string channel = message.parameters.at(0);
    std::string action = message.command == "JOIN" ? "joins" : "leaves";
    //printf("%s %s %s\n", message.prefix.nickname.c_str(), action.c_str(), channel.c_str());
}

void IRCClient::handleUserNickChange(IRCMessage message) {
    std::string newNick = message.parameters.at(0);
    //printf("%s changed his nick to %s\n", message.prefix.nickname.c_str(), newNick.c_str());
}

void IRCClient::handleUserQuit(IRCMessage message) {
    std::string text = message.parameters.at(0);
    //printf("%s quits (%s)\n", message.prefix.nickname.c_str(), text.c_str());
}

void IRCClient::handleChannelNamesList(IRCMessage message) {
    std::string channel = message.parameters.at(2);
    std::string nicks = message.parameters.at(3);
    //printf("<Channel[%s] people:\n%s\n", channel.c_str(), nicks.c_str());
}

void IRCClient::handleNicknameInUse(IRCMessage message) {
    //printf("%s %s\n", message.parameters.at(1).c_str(), message.parameters.at(2).c_str());
}

void IRCClient::handleServerMessage(IRCMessage message) {
    if (message.parameters.empty())
        return;

    //auto it = message.parameters.begin();
    //++it; // skip the first parameter (our nick)
    //for (; it != message.parameters.end(); ++it)
    //    printf("%s ", it->c_str());
    //printf("\n");
}
