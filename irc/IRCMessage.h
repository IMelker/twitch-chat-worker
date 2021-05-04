//
// Created by l2pic on 03.05.2021.
//

#ifndef CHATCONTROLLER_IRC_IRCMESSAGE_H_
#define CHATCONTROLLER_IRC_IRCMESSAGE_H_

#include <string>
#include <string_view>

#include "spdlog/fmt/ostr.h" // must be included

#include "Clock.h"

struct IRCMessage
{
    IRCMessage(std::string_view channel, std::string_view nickname, std::string_view text)
      : channel(channel), nickname(nickname), text(text) {
        timestamp = CurrentTime<std::chrono::system_clock>::milliseconds();
    }

    IRCMessage(IRCMessage&& other) = default;

    std::string channel;
    std::string nickname;
    std::string text;
    long long timestamp = 0;
};

inline std::ostream& operator<<(std::ostream& os, const IRCMessage& m) {
    return os << "{"
                 "channel: " << m.channel << ", "
                 "nickname: " << m.nickname << ", "
                 "text: \"" << m.text << "\""
                 "}";
}

#endif //CHATCONTROLLER_IRC_IRCMESSAGE_H_
