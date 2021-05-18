//
// Created by l2pic on 17.05.2021.
//

#ifndef CHATCONTROLLER__HTTPNOTIFIER_H_
#define CHATCONTROLLER__HTTPNOTIFIER_H_

#include "app.h"

#include <so_5/agent.hpp>
#include <so_5/timers.hpp>

#include <nlohmann/json.hpp>

#include "Config.h"
#include "URI.h"

#include "http/client/HTTPClient.h"
#include "http/client/HTTPRequestFactory.h"

#define GET_NOTIFIER_MBOX() so_environment().create_mbox(NOTIFIER)

using json = nlohmann::json;

class SlackNotifier {
  public:
    struct Text { std::string text; };
    struct Notify { std::string type; std::string text; };
  public:
    SlackNotifier() = default;
    explicit SlackNotifier(const std::string& hook) : hookUrl(hook) {
    }

    HTTPRequest text(const std::string& text) {
        auto body = json::object();
        body["text"] = text;
        return HTTPRequestFactory::Post(hookUrl, body.dump(), "application/json");
    }

    HTTPRequest notify(const std::string& type, const std::string& text) {
        auto body = json::object();
        body["blocks"] = {
            { {"type", "divider"} },
            {
                {"type", "section"},
                {"text", {
                    {"type", "plain_text"},
                    {"text", text}
                    }
                }
            },
            {
                {"type", "context"},
                {"elements", {
                    {{"type", "mrkdwn"},
                     {"text", "*" APP_NAME "* " APP_VERSION " " APP_GIT_HASH}},
                    {{"type", "mrkdwn"},
                     {"text", "*Type*: " + type}}
                    }
                }
            }
        };
        return HTTPRequestFactory::Post(hookUrl, body.dump(), "application/json");
    }

  private:
    URI hookUrl;
};

class Logger;
class HttpNotifier final : public so_5::agent_t
{
    struct ShutdownCheck final : public so_5::signal_t {};

  public:
    HttpNotifier(const context_t &ctx, Config &config, std::shared_ptr<Logger> logger);
    ~HttpNotifier() override;

    // so_5::agent_t implementation
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // notification evt
    void evtSlackText(so_5::mhood_t<SlackNotifier::Text> evt);
    void evtSlackNotify(so_5::mhood_t<SlackNotifier::Notify> evt);
  private:
    Config &config;

    const std::shared_ptr<Logger> logger;
    std::shared_ptr<HTTPClient> client;

    SlackNotifier slack;

    so_5::mbox_t self;
    so_5::timer_id_t shutdownCheckTimer;
};

#endif //CHATCONTROLLER__HTTPNOTIFIER_H_
