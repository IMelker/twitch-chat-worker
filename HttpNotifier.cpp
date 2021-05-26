//
// Created by l2pic on 17.05.2021.
//

#include <so_5/send_functions.hpp>

#include "SysSignal.h"
#include "Logger.h"

#include "http/details/RootCertificates.h"
#include "http/client/HTTPClientSession.h"
#include "HttpNotifier.h"

HttpNotifier::HttpNotifier(const so_5::agent_t::context_t &ctx, Config &config, std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), config(config), logger(std::move(logger)) {
    self = GET_NOTIFIER_MBOX();
}

HttpNotifier::~HttpNotifier() = default;

void HttpNotifier::so_define_agent() {
    so_subscribe_self().event([this](mhood_t<HttpNotifier::ShutdownCheck>) {
        if (SysSignal::serviceTerminated())
            so_deregister_agent_coop_normally();
    });

    so_subscribe(self).event(&HttpNotifier::evtSlackText, so_5::thread_safe);
    so_subscribe(self).event(&HttpNotifier::evtSlackNotify, so_5::thread_safe);
}

void HttpNotifier::so_evt_start() {
    slack = SlackNotifier{config[NOTIFIER]["slack_url"].value_or("")};

    HTTPClientConfig httpConfig;
    httpConfig.version = config[HTTP_CLIENT]["version"].value_or(11);
    httpConfig.userAgent = config[HTTP_CLIENT]["user_agent"].value_or(APP_NAME);
    httpConfig.timeout = config[HTTP_CLIENT]["timeout"].value_or(30);
    httpConfig.threads = config[HTTP_CLIENT]["threads"].value_or(1);
    httpConfig.secure = config[HTTP_CLIENT]["secure"].value_or(true);
    if (httpConfig.secure) {
        httpConfig.ssl.verify = config[HTTP_CLIENT]["verify"].value_or(false);
        httpConfig.ssl.cert = config[HTTP_CLIENT]["cert_file"].value_or("cert.pem");
        httpConfig.ssl.key = config[HTTP_CLIENT]["key_file"].value_or("key.pem");
        httpConfig.ssl.dh = config[HTTP_CLIENT]["dh_file"].value_or("dh.pem");
    }

    try {
        client = std::make_shared<HTTPClient>(std::move(httpConfig), this->logger);
        if (!client->start()) {
            throw;
        }
    } catch (...) {
        so_deregister_agent_coop(so_5::dereg_reason::user_defined_reason);
    }

    shutdownCheckTimer = so_5::send_periodic<HttpNotifier::ShutdownCheck>(so_direct_mbox(),
                                                                          std::chrono::seconds(1),
                                                                          std::chrono::seconds(1));

    if (canSendSlack()) client->exec(slack.notify(SlackNotifier::Type::Trace, "Application started: \"" APP_BIN_NAME "\""));
}

void HttpNotifier::so_evt_finish() {
    if (canSendSlack()) client->exec(slack.notify(SlackNotifier::Type::Trace, "Application shutdown: \"" APP_BIN_NAME "\""));
}

void HttpNotifier::evtSlackText(so_5::mhood_t<SlackNotifier::Text> evt) {
    if (canSendSlack()) client->exec(slack.text(evt->text));
}

void HttpNotifier::evtSlackNotify(so_5::mhood_t<SlackNotifier::Notify> evt) {
    if (canSendSlack()) client->exec(slack.notify(evt->type, evt->text));
}

bool HttpNotifier::canSendSlack() const {
    return client && slack.valid();
}
