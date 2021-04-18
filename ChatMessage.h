//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER__MESSAGE_H_
#define CHATSNIFFER__MESSAGE_H_

#include <string>
#include <string_view>
#include <utility>
#include "common/Utils.h"

#include "spdlog/fmt/ostr.h" // must be included

namespace Chat {

struct Message {
    Message(std::string user, std::string channel, std::string text,
            std::string lang, long long timestamp, bool valid)
        : uuid(Utils::UUIDv4::pair()), user(std::move(user)), channel(std::move(channel)), text(std::move(text)),
          lang(std::move(lang)), timestamp(timestamp), valid(valid) {
    }

    const std::pair<uint128_t, std::string> uuid;
    const std::string user;
    const std::string channel;
    const std::string text;
    const std::string lang;
    const long long timestamp;
    const bool valid;
};

inline std::ostream& operator<<(std::ostream& os, const Message& message) {
    return os << "{uuid: \"" << message.uuid.second <<
              "\", channel: \"" << message.channel <<
              "\", from \"" << message.user <<
              "\", text: \"" << message.text <<
              "\", lang: \"" << message.lang <<
              "\", valid: " << message.valid << "}";
}

}

#endif //CHATSNIFFER__MESSAGE_H_
