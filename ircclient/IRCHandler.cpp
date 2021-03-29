#include "app.h"
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

void IRCClient::handleCTCP(const IRCMessage& message) {
    std::string text{message.parameters.back()};

    // Remove '\001' from start/end of the string
    text = text.substr(1, text.size() - 2);

    //printf("handleCTCP: [%s requested CTCP %s]\n", message.prefix.nickname.c_str(), text.c_str());

    if (auto to = message.parameters[0]; to == nick) {
        // Respond to CTCP VERSION
        if (text == "VERSION") {
            std::string notice;
            notice.reserve(message.prefix.nickname.size() + sizeof("NOTICE  :\001VERSION " APP_NAME "-" APP_VERSION " \001\n"));
            notice.append("NOTICE ");
            notice.append(message.prefix.nickname);
            notice.append(" :\001VERSION " APP_NAME "-" APP_VERSION " \001\n");
            sendIRC(notice);
            return;
        }

        // CTCP not implemented
        std::string not_implemented;
        not_implemented.reserve(message.prefix.nickname.size() + text.size() +
                                sizeof("NOTICE  :\001ERRMSG  :Not implemented\001\n"));
        not_implemented.append("NOTICE ");
        not_implemented.append(message.prefix.nickname);
        not_implemented.append(" :\001ERRMSG ");
        not_implemented.append(text);
        not_implemented.append(" :Not implemented\001\n");
        sendIRC(not_implemented);
    }
}

void IRCClient::handlePrivMsg(const IRCMessage& message) {
    auto to = message.parameters[0];
    auto text = message.parameters.back();

    // Handle Client-To-Client Protocol
    if (text[0] == '\001') {
        handleCTCP(message);
        return;
    }

    /*if (to[0] == '#') {
        printf("handlePrivMsg: [%s] %s: %s\n", to.c_str(), message.prefix.nickname.c_str(), text.c_str());
    } else {
        printf("handlePrivMsg: %s: %s\n", message.prefix.nickname.c_str(), text.c_str());
    }*/
}

void IRCClient::handleNotice(const IRCMessage& message) {
    std::string text;
    if (!message.parameters.empty())
        text = message.parameters.back();

    auto from = message.prefix.nickname.empty() ? message.prefix.prefix : message.prefix.nickname;
    if (!text.empty() && text[0] == '\001') {
        text = text.substr(1, text.size() - 2);
        if (text.find(' ') == std::string::npos) {
            fprintf(stderr, "[Invalid %s reply from %*s]\n", text.c_str(), (int)from.size(), from.data());
            return;
        }
        std::string ctcp = text.substr(0, text.find(' '));
        //printf("handleNotice: [%s %s reply]: %s\n", from.c_str(), ctcp.c_str(), text.substr(text.find(' ') + 1).c_str());
    } else{
        //printf("handleNotice: %s- %s\n", from.c_str(), text.c_str());
    }
}

void IRCClient::handleChannelJoinPart(const IRCMessage& message) {
    auto channel = message.parameters.at(0);
    auto action = (message.command == "JOIN") ? "joins" : "leaves";
    //printf("handleChannelJoinPart: %s %s %s\n", message.prefix.nickname.c_str(), action.c_str(), channel.c_str());
}

void IRCClient::handleUserNickChange(const IRCMessage& message) {
    auto newNick = message.parameters.at(0);
    //printf("handleUserNickChange: %s changed his nick to %s\n", message.prefix.nickname.c_str(), newNick.c_str());
}

void IRCClient::handleUserQuit(const IRCMessage& message) {
    auto text = message.parameters.at(0);
    //printf("handleUserQuit: %s quits (%s)\n", message.prefix.nickname.c_str(), text.c_str());
}

void IRCClient::handleChannelNamesList(const IRCMessage& message) {
    auto channel = message.parameters.at(2);
    auto nicks = message.parameters.at(3);
    //printf("handleChannelNamesList: [%s] people:\n%s\n", channel.c_str(), nicks.c_str());
}

void IRCClient::handleNicknameInUse(const IRCMessage& message) {
    //printf("handleNicknameInUse: %s %s\n", message.parameters.at(1).c_str(), message.parameters.at(2).c_str());
}

void IRCClient::handleServerMessage(const IRCMessage& message) {
    if (message.parameters.empty())
        return;

    //auto it = message.parameters.begin();
    //++it; // skip the first parameter (our nick)
    //for (; it != message.parameters.end(); ++it)
    //    printf("%s ", it->c_str());
    //printf("\n");
}
