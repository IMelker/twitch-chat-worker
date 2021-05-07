//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER__MESSAGE_H_
#define CHATSNIFFER__MESSAGE_H_

#include <string>
#include <utility>

#include "common/Utils.h"

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

struct SendMessage { std::string account; mutable std::string channel; mutable std::string text; };

}

#endif //CHATSNIFFER__MESSAGE_H_
