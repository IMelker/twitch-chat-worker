//
// Created by l2pic on 14.04.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCEVENTS_H_
#define CHATCONTROLLER_IRC_IRCEVENTS_H_

#include <string>

class IRCWorker;

struct WorkerConnected { IRCWorker *worker = nullptr; };
struct WorkerDisconnected { IRCWorker *worker = nullptr; };
struct WorkerLoggedIn { IRCWorker *worker = nullptr; };

struct JoinChannel { std::string channel; };
struct LeaveChannel { std::string channel; };
struct SendMessage { std::string account; std::string channel; std::string text; };
struct SendIRC { std::string message; };
struct SendPING { std::string host; };

#endif //CHATCONTROLLER_IRC_IRCEVENTS_H_
