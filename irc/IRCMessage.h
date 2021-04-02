//
// Created by imelker on 29.03.2021.
//

#ifndef CHATSNIFFER_IRCCLIENT_IRCMESSAGE_H_
#define CHATSNIFFER_IRCCLIENT_IRCMESSAGE_H_

#include <string>
#include <string_view>
#include <iterator>
#include <vector>
#include "spdlog/fmt/ostr.h" // must be included

#define MAX_MESSAGE_LEN 1024

struct IRCPrefix
{
    IRCPrefix() = default;
    IRCPrefix(const char* data, size_t len) : prefix(data, len) {
        std::vector<std::string_view> tokens = absl::StrSplit(this->prefix, '@', absl::SkipWhitespace{});
        if (tokens.size() > 1) {
            this->nickname = tokens[0];
            this->host = tokens[1];
        }
        tokens = absl::StrSplit(this->nickname, '!', absl::SkipWhitespace{});
        if (tokens.size() > 1) {
            this->nickname = tokens[0];
            this->username = tokens[1];
        }
    }

    std::string prefix;
    std::string nickname;
    std::string username;
    std::string host;
};

class IRCMessage
{
    std::string payload;
  public:
    IRCMessage() = default;
    IRCMessage(const char* data, size_t len) : payload(data, static_cast<size_t>(len)) {
        size_t next_space = payload.find(' ');
        if (payload.front() == ':')
            prefix = IRCPrefix{payload.data() + 1, next_space - 1};

        size_t command_end = payload.find(' ', next_space + 1);
        if (command_end == std::string::npos) {
            command = payload.substr(next_space + 1);
        } else {
            command = payload.substr(next_space + 1, command_end - next_space - 1);

            if (payload.at(command_end + 1) == ':') {
                parameters.push_back(payload.substr(command_end + 2));
            } else {
                size_t pos1 = command_end + 1, pos2;
                while ((pos2 = payload.find(' ', pos1)) != std::string::npos) {
                    parameters.push_back(payload.substr(pos1, pos2 - pos1));
                    pos1 = pos2 + 1;
                    if (payload[pos1] == ':') {
                        parameters.push_back(payload.substr(pos1 + 1));
                        break;
                    }
                }
                if (parameters.empty())
                    parameters.push_back(payload.substr(command_end + 1));
            }
        }
        std::transform(command.begin(), command.end(), command.begin(), ::towupper);
    }

    IRCPrefix prefix;
    std::string command;
    std::vector<std::string> parameters{};
};

inline std::ostream& operator<<(std::ostream& os, const IRCPrefix& p) {
    os << "{";
    if (!p.username.empty())
        os << " username: " << p.username << ",";
    if (!p.host.empty())
        os << " host: " << p.host << ",";
    if (!p.nickname.empty())
        os << " nickname: " << p.nickname;
    return os << "}";
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<std::string> &p) {
    std::copy(p.begin(), p.end(), std::ostream_iterator<std::string>(os, ", "));
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const IRCMessage& m) {
    os << "{";
    if (!m.prefix.prefix.empty())
        os << "prefix:" << m.prefix << ", ";
    return os << "command: " << m.command << ", parameters: " << m.parameters << "}";
}

#endif //CHATSNIFFER_IRCCLIENT_IRCMESSAGE_H_
