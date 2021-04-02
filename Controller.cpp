//
// Created by l2pic on 06.03.2021.
//
#include "app.h"

#include <algorithm>
#include <chrono>

#include "nlohmann/json.hpp"

#include "common/Utils.h"
#include "common/SysSignal.h"
#include "common/Logger.h"
#include "common/LoggerFactory.h"

#include "db/DBConnectionLock.h"
#include "Controller.h"

using json = nlohmann::json;

Controller::Controller(Config &config, ThreadPool *pool, std::shared_ptr<Logger> logger)
    : pool(pool), config(config), logger(std::move(logger)) {
    this->logger->logInfo("Controller created with {} threads", pool->size());

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
    auto httpLogger = LoggerFactory::create(LoggerFactory::config(config, HTTP_CONTROL));
    httpServer = std::make_shared<HTTPServer>(std::move(httpCfg), httpLogger);
    httpServer->addControlUnit(APP, this);
}

Controller::~Controller() {
    logger->logTrace("Controller end of controller");
    httpServer->clearUnits();
};

bool Controller::startHttpServer() {
    return httpServer->start(pool);
}

bool Controller::initDBStorage() {
    assert(!db);

    PGConnectionConfig pgCfg;
    pgCfg.host = config[POSTGRESQL]["host"].value_or("localhost");
    pgCfg.port = config[POSTGRESQL]["port"].value_or(5432);
    pgCfg.dbname = config[POSTGRESQL]["dbname"].value_or("postgres");
    pgCfg.user = config[POSTGRESQL]["user"].value_or("postgres");
    pgCfg.pass = config[POSTGRESQL]["password"].value_or("postgres");
    int pgConns = config[POSTGRESQL]["connections"].value_or(std::thread::hardware_concurrency());
    auto pgLogger = LoggerFactory::create(LoggerFactory::config(config, POSTGRESQL));

    db = std::make_shared<DBStorage>(std::move(pgCfg), pgConns, pgLogger);
    httpServer->addControlUnit(POSTGRESQL, db->getPGPool());

    return db->getPGPool()->size() > 0;
}

bool Controller::initIRCWorkerPool() {
    assert(msgProcessor); // as IRCMessageListener
    assert(!ircWorkers);

    IRCConnectConfig ircConfig;
    ircConfig.host = config[IRC]["host"].value_or("irc.chat.twitch.tv");
    ircConfig.port = config[IRC]["port"].value_or(6667);

    auto ircLogger = LoggerFactory::create(LoggerFactory::config(config, IRC));
    ircWorkers = std::make_shared<IRCWorkerPool>(ircConfig, db.get(), msgProcessor.get(), ircLogger);
    httpServer->addControlUnit(IRC, ircWorkers.get());

    return ircWorkers->poolSize() > 0;
}

bool Controller::initMessageProcessor() {
    assert(!msgProcessor);
    msgProcessor = std::make_shared<MessageProcessor>(pool, logger);
    return true;
}

bool Controller::initMessageStorage() {
    assert(msgProcessor); // as publisher
    assert(!msgStorage);

    CHConnectionConfig chCfg;
    chCfg.host = config[CLICKHOUSE]["host"].value_or("localhost");
    chCfg.port = config[CLICKHOUSE]["port"].value_or(5432);
    chCfg.dbname = config[CLICKHOUSE]["dbname"].value_or("postgres");
    chCfg.user = config[CLICKHOUSE]["user"].value_or("postgres");
    chCfg.pass = config[CLICKHOUSE]["password"].value_or("postgres");
    chCfg.secure = config[CLICKHOUSE]["secure"].value_or(false);
    chCfg.verify = config[CLICKHOUSE]["verify"].value_or(true);
    chCfg.sendRetries = config[CLICKHOUSE]["send_retries"].value_or(5);
    int chConns = config[CLICKHOUSE]["connections"].value_or(std::thread::hardware_concurrency());
    auto chLogger = LoggerFactory::create(LoggerFactory::config(config, CLICKHOUSE));

    msgStorage = std::make_shared<MessageStorage>(std::move(chCfg), chConns, pool, chLogger);

    httpServer->addControlUnit(CLICKHOUSE, msgStorage->getCHPool());
    msgProcessor->subscribe(msgStorage.get());

    return msgStorage->getCHPool()->size() > 0;
}

bool Controller::initBotsEnvironment() {
    assert(msgProcessor); // as publisher
    assert(!botsEnvironment);

    auto botsLogger = LoggerFactory::create(LoggerFactory::config(config, BOT));
    botsEnvironment = std::make_shared<BotsEnvironment>(botsLogger);

    httpServer->addControlUnit(BOT, botsEnvironment.get());
    msgProcessor->subscribe(botsEnvironment.get());

    return true;
}

void Controller::loop() {
    while (!SysSignal::serviceTerminated()){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::tuple<int, std::string> Controller::processHttpRequest(std::string_view path, const std::string &request, std::string &error) {
    if (path == "version")
        return {200, handleVersion(request, error)};
    if (path == "shutdown")
        return {200, handleShutdown(request, error)};

    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}

std::string Controller::handleShutdown(const std::string &, std::string &) {
    SysSignal::setServiceTerminated(true);
    return "Terminated";
}

std::string Controller::handleVersion(const std::string &, std::string &) {
    json body = json::object();
    body["version"] = APP_NAME " " APP_VERSION " " APP_GIT_DATE " (" APP_GIT_HASH ")";
    return body.dump();
}
