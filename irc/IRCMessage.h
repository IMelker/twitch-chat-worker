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

#include "../common/Clock.h"

#define MAX_MESSAGE_LEN 1024

/*
 * From RFC 1459:
 *  <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
 *  <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
 *  <command>  ::= <letter> { <letter> } | <number> <number> <number>
 *  <SPACE>    ::= ' ' { ' ' }
 *  <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
 *  <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
 *                 or NUL or CR or LF, the first of which may not be ':'>
 *  <trailing> ::= <Any, possibly *empty*, sequence of octets not including
 *                   NUL or CR or LF>
 */

struct IRCPrefix
{
    IRCPrefix() = default;
    IRCPrefix(const char* data, size_t len) : prefix(data, len) {
        parse(prefix);
    }

    std::string prefix;
    std::string nickname;
    std::string username;
    std::string host;
  private:
    void parse(const std::string& data) {
        std::vector<std::string_view> tokens = absl::StrSplit(data, '@', absl::SkipWhitespace{});
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
};

class IRCMessage
{
    std::string payload;
  public:
    IRCMessage() = default;
    IRCMessage(const char* data, size_t len)
      : payload(data, static_cast<size_t>(len)),
        timestamp(CurrentTime<std::chrono::system_clock>::milliseconds()) {
        parse(payload);
    }

    IRCPrefix prefix;
    std::string command;
    std::vector<std::string> parameters{};
    long long timestamp{};
  private:
    void parse(const std::string& data) {
        size_t next_space = data.find(' ');
        if (data.front() == ':')
            prefix = IRCPrefix{data.data() + 1, next_space - 1};

        size_t command_end = data.find(' ', next_space + 1);
        if (command_end == std::string::npos) {
            command = data.substr(next_space + 1);
        } else {
            command = data.substr(next_space + 1, command_end - next_space - 1);

            if (data.at(command_end + 1) == ':') {
                parameters.push_back(data.substr(command_end + 2));
            } else {
                size_t pos1 = command_end + 1, pos2;
                while ((pos2 = data.find(' ', pos1)) != std::string::npos) {
                    parameters.push_back(data.substr(pos1, pos2 - pos1));
                    pos1 = pos2 + 1;
                    if (data[pos1] == ':') {
                        parameters.push_back(data.substr(pos1 + 1));
                        break;
                    }
                }
                if (parameters.empty())
                    parameters.push_back(data.substr(command_end + 1));
            }
        }
        std::transform(command.begin(), command.end(), command.begin(), ::towupper);
    }
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
