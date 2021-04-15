//
// Created by l2pic on 15.04.2021.
//
#include "app.h"
#include "HttpController.h"

HttpController::HttpController(const context_t &ctx, Config &config, std::shared_ptr<Logger> logger)
    : so_5::agent_t(ctx), config(config), logger(std::move(logger)) {
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
    server = std::make_shared<HTTPServer>(std::move(httpCfg), this->logger);
}

HttpController::~HttpController() = default;

void HttpController::so_define_agent() {

}

void HttpController::so_evt_start() {

}

void HttpController::so_evt_finish() {

}
