//
// Created by l2pic on 03.04.2021.
//

#ifndef CHATSNIFFER_BOT_BOTCONFIGURATION_H_
#define CHATSNIFFER_BOT_BOTCONFIGURATION_H_

#include <cassert>
#include <string>
#include <vector>

#include <pcrecpp.h>
#include <nlohmann/json.hpp>

#include "../ChatMessage.h"

using json = nlohmann::json;

enum class BotEventType : int {
    Message = 1,
    Interval = 2,
    Timer = 3
};

class BotEventHandler {
  public:
    BotEventHandler(int id, std::string&& script, std::string&& additional)
      : id(id), script(std::move(script)), additional(std::move(additional)) {

    }
    ~BotEventHandler() = default;

    int getId() const { return id; }
    const std::string& getScript() const { return script; }
    const std::string& getAdditional() const { return additional; }

  protected:
    int id = 0;
    std::string script;
    std::string additional;
};

class BotMessageHandler : public BotEventHandler {
  public:
    BotMessageHandler(int id, std::string&& script, std::string&& additional)
      : BotEventHandler(id, std::move(script), std::move(additional)), text(""), user("") {
        json add = json::parse(this->additional, nullptr, false, true);
        valid = !add.is_discarded();

        if (valid) {
            pcrecpp::RE_Options options;
            options.set_utf8(true);
            auto &route = add["route"];
            if (!route.is_null()) {
                auto &t = route["text"];
                if (t.is_string())
                    text = pcrecpp::RE(t.get<std::string>(), options);
                auto &u = route["user"];
                if (u.is_string())
                    user = pcrecpp::RE(u.get<std::string>(), options);
            }
        }
    }
    ~BotMessageHandler() = default;

    [[nodiscard]] bool match(const Chat::Message& msg) const {
        if (!valid)
            return false;

        if (!text.pattern().empty()) {
            if (!text.error().empty() || !text.PartialMatch(msg.text))
                return false;
        }
        if (!user.pattern().empty()) {
            if (!user.error().empty() || !user.PartialMatch(msg.user))
                return false;
        }
        return true;
    };
  private:
    bool valid = true;
    pcrecpp::RE text;
    pcrecpp::RE user;
};

struct BotConfiguration {
    int botId = 0;
    int userId = 0;
    std::string account;
    std::string channel;

    std::vector<BotMessageHandler> onMessage;
    std::vector<BotEventHandler> onInterval;
    std::vector<BotEventHandler> onTimer;
};

#endif //CHATSNIFFER_BOT_BOTCONFIGURATION_H_
