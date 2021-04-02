#include "app.h"

#include <fmt/format.h>

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

