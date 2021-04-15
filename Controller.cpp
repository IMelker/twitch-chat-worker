//
// Created by imelker on 06.03.2021.
//
#include "app.h"

#include <algorithm>
#include <chrono>

#include <so_5/send_functions.hpp>
#include <so_5/disp/thread_pool/pub.hpp>

#include "nlohmann/json.hpp"

#include "common/Utils.h"
#include "common/SysSignal.h"
#include "common/Logger.h"
#include "common/LoggerFactory.h"

#include "db/DBConnectionLock.h"
#include "Controller.h"

using json = nlohmann::json;

Controller::Controller(const context_t& ctx, so_5::mbox_t http, Config &config,
                       std::shared_ptr<DBController> db, std::shared_ptr<Logger> logger)
    : so_5::agent_t(ctx),
      config(config),
      logger(std::move(logger)),
      db(std::move(db)),
      http(std::move(http)) {
}

Controller::~Controller() {
    logger->logTrace("Controller end of controller");
};

void Controller::so_define_agent() {
    so_subscribe_self().event([this](mhood_t<Controller::ShutdownCheck>) {
        if (SysSignal::serviceTerminated()) {
            so_deregister_agent_coop_normally();
        }
    });
}

void Controller::so_evt_start() {
    so_5::introduce_child_coop(*this, [&] (so_5::coop_t &coop) {
        auto listener = so_environment().create_mbox();
        storage = makeStorage(coop, listener);
        botsEnvironment = makeBotsEnvironment(coop, listener);
        msgProcessor = makeMessageProcessor(coop, listener /*as publisher*/);
        ircWorkers = makeIRCWorkerPool(coop);

        botsEnvironment->setMessageSender(ircWorkers->so_direct_mbox());
        botsEnvironment->setBotLogger(storage->so_direct_mbox());
    });

    shutdownCheckTimer = so_5::send_periodic<Controller::ShutdownCheck>(so_direct_mbox(),
                                                                        std::chrono::seconds(1),
                                                                        std::chrono::seconds(1));
}

void Controller::so_evt_finish() {

}


Storage * Controller::makeStorage(so_5::coop_t &coop, const so_5::mbox_t& listener) {
    CHConnectionConfig chCfg;
    chCfg.host = config[CLICKHOUSE]["host"].value_or("localhost");
    chCfg.port = config[CLICKHOUSE]["port"].value_or(5432);
    chCfg.dbname = config[CLICKHOUSE]["dbname"].value_or("postgres");
    chCfg.user = config[CLICKHOUSE]["user"].value_or("postgres");
    chCfg.pass = config[CLICKHOUSE]["password"].value_or("postgres");
    chCfg.secure = config[CLICKHOUSE]["secure"].value_or(false);
    chCfg.verify = config[CLICKHOUSE]["verify"].value_or(true);
    chCfg.sendRetries = config[CLICKHOUSE]["send_retries"].value_or(5);
    int batchSize = config[CLICKHOUSE]["batch_size"].value_or(1000);
    unsigned int chConns = config[CLICKHOUSE]["connections"].value_or(1);

    auto chLogger = LoggerFactory::create(LoggerFactory::config(config, CLICKHOUSE));
    return coop.make_agent<Storage>(listener, std::move(chCfg), chConns, batchSize, chLogger);
}

BotsEnvironment *Controller::makeBotsEnvironment(so_5::coop_t &coop, const so_5::mbox_t& listener) {
    auto botsLogger = LoggerFactory::create(LoggerFactory::config(config, BOT));
    return coop.make_agent<BotsEnvironment>(listener, db.get(), botsLogger);
}

MessageProcessor *Controller::makeMessageProcessor(so_5::coop_t &coop, const so_5::mbox_t& publisher) {
    MessageProcessorConfig procCfg;
    procCfg.languageRecognition = config[MSG]["language_recognition"].value_or(false);
    unsigned int procThreads = config[MSG]["threads"].value_or(2);

    auto procPool = so_5::disp::thread_pool::make_dispatcher(so_environment(), procThreads);
    auto procPoolParams = so_5::disp::thread_pool::bind_params_t{};
    return coop.make_agent_with_binder<MessageProcessor>(procPool.binder(procPoolParams),
                                                         publisher, std::move(procCfg), this->logger);
}

IRCWorkerPool *Controller::makeIRCWorkerPool(so_5::coop_t &coop) {
    IRCConnectConfig ircConfig;
    ircConfig.host = config[IRC]["host"].value_or("irc.chat.twitch.tv");
    ircConfig.port = config[IRC]["port"].value_or(6667);
    auto ircLogger = LoggerFactory::create(LoggerFactory::config(config, IRC));
    return coop.make_agent<IRCWorkerPool>(msgProcessor->so_direct_mbox(), ircConfig, db.get(), ircLogger);
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
