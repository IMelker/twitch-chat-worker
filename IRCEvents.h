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

class IRCWorker;
namespace Irc {

struct Shutdown final : so_5::signal_t {};

// Message emitted on worker connected
struct WorkerConnected {
    IRCWorker *worker = nullptr;
};

// Message emitted on worker disconnected
struct WorkerDisconnected {
    IRCWorker *worker = nullptr;
};

// Message emitted on worker logged in
struct WorkerLoggedIn {
    IRCWorker *worker = nullptr;
};

// Signal that emits IRCWorker to get channels
struct InitAutoJoin : public so_5::signal_t {};

// Message for IRCWorker to ask for channels
struct GetChannels {
    IRCWorker *worker = nullptr;
    unsigned int count = 0;
};

// Load from DB channels list, init LeaveAllChannels on workers, init auto join
struct ReloadChannels : public so_5::signal_t {};

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
    IRCWorker *worker = nullptr;
};

// Message for channel joining on exact worker
struct JoinChannelOnAccount : public JoinChannel {
    IRCWorker *worker = nullptr;
};

// Message for channel leaving
struct LeaveChannel {
    std::string channel;
};

// Message for leaving all channels without notify
struct LeaveAllChannels : public so_5::signal_t {};

// Command to send IRC text message
struct SendMessage { std::string account; std::string channel; std::string text; };

// Command to send custom IRC message
struct SendIRC { std::string message; };

// Command to send PING
struct SendPING { std::string host; };

}

#endif //CHATCONTROLLER__IRCEVENTS_H_
