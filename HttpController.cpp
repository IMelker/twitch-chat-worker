//
// Created by l2pic on 15.04.2021.
//
#include "app.h"

#include <so_5/send_functions.hpp>

#include "absl/strings/str_split.h"

#include "common/SysSignal.h"
#include "http/server/HTTPResponseFactory.h"
#include "DBController.h"
#include "HttpController.h"

#define match(num, arg) path[num] == #arg
#define match_handle2(arg1, arg2) if (path[1] == #arg2) return so_5::send<hreq::arg1::arg2>(listeners, std::move(req), std::move(send))
#define match_handle3(arg1, arg2, arg3) if (path[2] == #arg3) return so_5::send<hreq::arg1::arg2::arg3>(listeners, std::move(req), std::move(send))

HttpController::HttpController(const context_t &ctx,
                               so_5::mbox_t listeners,
                               Config &config,
                               std::shared_ptr<DBController> db,
                               std::shared_ptr<Logger> logger)
    : so_5::agent_t(ctx), config(config), db(std::move(db)),
      logger(std::move(logger)), listeners(std::move(listeners)) {
}

HttpController::~HttpController() = default;

void HttpController::so_define_agent() {
    so_subscribe_self().event([this](mhood_t<HttpController::ShutdownCheck>) {
        if (SysSignal::serviceTerminated()) {
            so_deregister_agent_coop_normally();
        }
    });
    so_subscribe(listeners).event([](mhood_t<hreq::resp> resp) {
        resp->send(HTTPResponseFactory::CreateResponse(
            resp->req, static_cast<http::status>(resp->status), std::move(resp->body)));
    }, so_5::thread_safe);
    so_subscribe(listeners).event(&HttpController::evtHttpDBControllerStatus);
}

void HttpController::so_evt_start() {
    HTTPControlConfig httpCfg;
    httpCfg.host = config[HTTP_CONTROL]["host"].value_or("localhost");
    httpCfg.port = config[HTTP_CONTROL]["port"].value_or(8080);
    httpCfg.user = config[HTTP_CONTROL]["user"].value_or("admin");
    httpCfg.pass = config[HTTP_CONTROL]["password"].value_or("admin");
    httpCfg.threads = config[HTTP_CONTROL]["threads"].value_or(std::thread::hardware_concurrency());
    httpCfg.secure = config[HTTP_CONTROL]["secure"].value_or(true);
    if (httpCfg.secure) {
        httpCfg.ssl.verify = config[HTTP_CONTROL]["verify"].value_or(false);
        httpCfg.ssl.cert = config[HTTP_CONTROL]["cert_file"].value_or("cert.pem");
        httpCfg.ssl.key = config[HTTP_CONTROL]["key_file"].value_or("key.pem");
        httpCfg.ssl.dh = config[HTTP_CONTROL]["dh_file"].value_or("dh.pem");
    }
    try {
        server = std::make_shared<HTTPServer>(std::move(httpCfg), this, this->logger);
        if (!server->start()) {
            throw;
        }
    } catch (...) {
        so_deregister_agent_coop(so_5::dereg_reason::user_defined_reason);
    }

    shutdownCheckTimer = so_5::send_periodic<HttpController::ShutdownCheck>(so_direct_mbox(),
                                                                            std::chrono::seconds(1),
                                                                            std::chrono::seconds(1));
}

void HttpController::so_evt_finish() {
    if (server)
        server->stop();
}

void HttpController::evtHttpDBControllerStatus(so_5::mhood_t<hreq::db::stats> req) {
    so_5::send<hreq::resp>(listeners, std::move(req->req), std::move(req->send), 200, db->getStats());
}

void HttpController::handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) {
    std::vector<std::string_view> path = absl::StrSplit(sv(req.target()), '/', absl::SkipWhitespace());
    if (path.size() < 2)
        return send(HTTPResponseFactory::BadRequest(req, "Invalid path"));

    if (match(0, storage)) {
        match_handle2(storage, stats);
    }
    else
    if (match(0, db)) {
        match_handle2(db, stats);
    }
    else
    if (match(0, irc)) {
        if (path.size() < 3) {
            send(HTTPResponseFactory::BadRequest(req, "Invalid path"));
            return;
        }
        if (match(1, channel)) {
            match_handle3(irc, channel, stats);
            else match_handle3(irc, channel, reload);
            else match_handle3(irc, channel, join);
            else match_handle3(irc, channel, leave);
            else match_handle3(irc, channel, message);
            else match_handle3(irc, channel, custom);
        }
        else if (match(1, account)) {
            match_handle3(irc, account, stats);
            else match_handle3(irc, account, reload);
        }
    }
    else
    if (match(0, app)) {
        match_handle2(app, shutdown);
        else match_handle2(app, version);
    }
    else
    if (match(0, bot)) {
        match_handle2(bot, add);
        else match_handle2(bot, remove);
        else match_handle2(bot, reload);
        else match_handle2(bot, reloadall);
    }

    send(HTTPResponseFactory::NotFound(req, req.target()));
}
