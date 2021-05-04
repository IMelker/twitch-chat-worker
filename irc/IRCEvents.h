//
// Created by l2pic on 19.04.2021.
//

#ifndef CHATCONTROLLER__IRCEVENTS_H_
#define CHATCONTROLLER__IRCEVENTS_H_

#include <string>
#include <vector>
#include <set>

#include <so_5/message.hpp>
#include <so_5/mbox.hpp>

class IRCClient;
namespace Irc {

struct Shutdown final : so_5::signal_t {};

// Message for ChannelController to answer for GetChannels
struct JoinChannels {
    std::vector<std::string> channels;
};

// Message for channel joining
struct JoinChannel {
    std::string channel;
};

// Message to report channel joined
struct ChannelJoined : public JoinChannel {
    IRCClient *client = nullptr;
};

// Message for channel joining on exact worker
struct JoinChannelOnAccount : public JoinChannel {
    IRCClient *client = nullptr;
};

// Message for channel leaving
struct LeaveChannel {
    std::string channel;
};

// Command to send IRC text message
struct SendMessage { std::string account; std::string channel; std::string text; };

// Command to send custom IRC message
struct SendIRC { std::string message; };

// Command to send PING
struct SendPING { std::string host; };

}

#endif //CHATCONTROLLER__IRCEVENTS_H_
