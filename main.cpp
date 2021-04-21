#include "app.h"

#include <cstdio>
#include <unistd.h>
#include <wait.h>
#include <thread>

#include <so_5/all.hpp>

#include "common/SysSignal.h"
#include "common/Options.h"
#include "common/Config.h"
#include "common/LoggerFactory.h"

#include "IRCWorker.h"
#include "db/ch/CHConnectionPool.h"
#include "db/pg/PGConnectionPool.h"

#include "HttpController.h"
#include "DBController.h"
#include "Controller.h"

#define UNIT_OK 0
#define UNIT_RESTART 1
#define UNIT_STOP (-1)

void printVersion() {
    printf("Name: " APP_NAME "\nVersion: " APP_VERSION "\nAuthor: " APP_AUTHOR_EMAIL "\n"
           "Git: " APP_GIT_HASH "\nBuild: " APP_GIT_DATE " gcc-" __VERSION__ "\n");
}

std::shared_ptr<DBController> createDBController(Config& config) {
    PGConnectionConfig pgCfg;
    pgCfg.host = config[POSTGRESQL]["host"].value_or("localhost");
    pgCfg.port = config[POSTGRESQL]["port"].value_or(5432);
    pgCfg.dbname = config[POSTGRESQL]["dbname"].value_or("postgres");
    pgCfg.user = config[POSTGRESQL]["user"].value_or("postgres");
    pgCfg.pass = config[POSTGRESQL]["password"].value_or("postgres");
    int pgConns = config[POSTGRESQL]["connections"].value_or(std::thread::hardware_concurrency());
    auto pgLogger = LoggerFactory::create(LoggerFactory::config(config, POSTGRESQL));

    return std::make_shared<DBController>(std::move(pgCfg), pgConns, pgLogger);
}

int main(int argc, char *argv[]) {
    SysSignal::setupSignalHandling();

    Options options{APP_NAME};
    options.addOption<std::string>("config", "c", "Config filepath", "config.toml");
    options.addOption<std::string>("pid-file-name", "p", "Filepath for process ID output", "/var/run/chatcontroller.pid");
    options.addOption<bool>("daemon", "d", "Daemon mode", "false");
    options.addOption<bool>("version", "v", "App version", "false");
    options.addOption<bool>("trace-exit-code", "t", "Trace exit code for daemon mode", "false");
    options.parse(argc, argv);

    if (options.getValue<bool>("version")) {
        printVersion();
        return UNIT_OK;
    }

    // daemon mode
    auto daemon = options.getValue<bool>("daemon");
    auto traceExitCode = options.getValue<bool>("trace-exit-code");
    auto pidFilename = options.getValue<std::string>("pid-file-name");
    if (daemon) {
        pid_t pid = fork();
        if (pid == 0) {
            setsid();

            FILE *pidFile = fopen(pidFilename.c_str(), "w+");
            fprintf(pidFile, "%d", getpid());
            fclose(pidFile);
        } else if (pid > 0) {
            if (traceExitCode) {
                int status;
                waitpid(pid, &status, 0);
                printf("Exit code is %d\n", status);
            }
            return UNIT_OK;
        } else {
            puts("Can't start as a daemon\n");
            return UNIT_STOP;
        }
    }

    Config config{options.getValue<std::string>("config")};

    auto appLogger = LoggerFactory::create(LoggerFactory::config(config, APP));
    auto httpLogger = LoggerFactory::create(LoggerFactory::config(config, HTTP_CONTROL));
    DefaultLogger::setAsDefault(appLogger);

    auto db = createDBController(config);
    if (db->getPGPool()->size() == 0) {
        appLogger->logCritical("Failed to initialize DBController. Exit");
        return UNIT_RESTART;
    }

    try {
        so_5::launch([&](so_5::environment_t &env) {
            auto httpBox = env.create_mbox();
            env.register_agent_as_coop(env.make_agent<Controller>(httpBox, config, db, appLogger));
            env.register_agent_as_coop(env.make_agent<HttpController>(httpBox, config, db, httpLogger));
        });
    }
    catch (const std::exception &e) {
        appLogger->logCritical("Exception: ", e.what());
        return UNIT_RESTART;
    }
    return UNIT_OK;

    // 1. TODO HTTPServer to IRCController
    // 2. TODO choose dispatchers/threadpools
    // 3. TODO add logging

    // TODO add so_5::state for IRCWorker, refactorings

    // TODO fix options, remove useless and add controlls

    // BotEngine
    // TODO BotEngine timer events
    // TODO add timer and timer event to bot
    // TODO move language detect to BOTEngine (?) // to decrease cpu usage

    // IRCWorkersPool
    // TODO change channel join behavior
    // TODO accounts update
    // TODO TCP selector for socket for multiple sockets
    // TODO IRCMessage to string_view, or something like that

    // Global
    // TODO Review accounts, channels, bots logic
    // TODO add tags support
    // TODO add key value storage
    // TODO add HTTPClient
    // TODO add TwitchAPI
    // TODO add smileys detection for messages

    // Blockings and users
    // TODO blocked date for bot
    // TODO blocked user for bot

    // Clickhouse
    // TODO set TTL for user_logs messages to 30 days https://clickhouse.tech/docs/en/engines/table-engines/mergetree-family/mergetree/#table_engine-mergetree-ttl
    // TODO make clickhouse SSL client
    // TODO change CH name and display sizes user max=50
}
