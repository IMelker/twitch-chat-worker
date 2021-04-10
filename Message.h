//
// Created by imelker on 02.04.2021.
//

#ifndef CHATSNIFFER__MESSAGE_H_
#define CHATSNIFFER__MESSAGE_H_

#include <string>
#include <utility>

struct Message {
    Message(std::string user, std::string channel, std::string text,
            std::string lang, long long timestamp, bool valid)
      : user(std::move(user)), channel(std::move(channel)), text(std::move(text)),
        lang(std::move(lang)), timestamp(timestamp), valid(valid) {
    }

    const std::string user;
    const std::string channel;
    const std::string text;
    const std::string lang;
    const long long timestamp;
    const bool valid;
};

#endif //CHATSNIFFER__MESSAGE_H_
