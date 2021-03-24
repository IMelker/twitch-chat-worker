#include "app.h"

#include <cstdio>
#include <unistd.h>
#include <wait.h>
#include <algorithm>
#include <map>
#include <thread>

#include "common/SysSignal.h"
#include "common/Options.h"
#include "common/Config.h"
#include "common/LoggerFactory.h"

#include "http/server/HTTPServer.h"
#include "ircclient/IRCWorker.h"
#include "db/ch/CHConnectionPool.h"
#include "db/pg/PGConnectionPool.h"

#include "IRCtoDBConnector.h"

#define APP "app"
#define HTTP_CONTROL "control"
#define POSTGRESQL "pg"
#define CLICKHOUSE "ch"
#define IRC "irc"

inline LoggerConfig readLoggerConfig(Config& config, const std::string& category) {
    LoggerConfig logConfig;
    logConfig.name = category;
    logConfig.type = LoggerFactory::typeFromString(config[category]["log_type"].value_or("file"));
    logConfig.level = LoggerFactory::levelFromString(config[category]["log_level"].value_or("info"));
    logConfig.target = config[category]["log_target"].value_or(category + ".log");
    logConfig.flushEvery = config[category]["flush_every"].value_or(10);
    logConfig.flushOn = LoggerFactory::levelFromString(config[category]["flush_on"].value_or("error"));
    return logConfig;
}

int main(int argc, char *argv[]) {
    SysSignal::setupSignalHandling();

    Options options{APP_NAME};
    options.addOption<std::string>("config", "c", "Config filepath", "config.toml");
    options.addOption<std::string>("pid-file-name", "p", "Filepath for process ID output", "/var/run/chatsniffer.pid");
    options.addOption<bool>("daemon", "d", "Daemon mode", "false");
    options.addOption<bool>("trace-exit-code", "t", "Trace exit code for daemon mode", "false");
    options.parse(argc, argv);

    // daemon mode
    auto daemon = options.getValue<bool>("daemon");
    auto traceExitCode = options.getValue<bool>("trace-exit-code");
    auto pidFilename = options.getValue<std::string>("pid-file-name");
    if (daemon) {
        int r = fork();
        if (r == 0) {
            FILE *pidFile = fopen(pidFilename.c_str(), "w+");
            fprintf(pidFile, "%d", getpid());
            fclose(pidFile);
        } else if (r > 0) {
            if (traceExitCode) {
                int status;
                waitpid(r, &status, 0);
                printf("Exit code is %d\n", status);
            }
            return 0;
        } else {
            puts("Can't start as a daemon\n");
            return 1;
        }
    }

    Config config{options.getValue<std::string>("config")};

    HTTPControlConfig httpCfg;
    httpCfg.host = config[HTTP_CONTROL]["host"].value_or("localhost");
    httpCfg.port = config[HTTP_CONTROL]["port"].value_or(8080);
    httpCfg.user = config[HTTP_CONTROL]["user"].value_or("admin");
    httpCfg.pass = config[HTTP_CONTROL]["password"].value_or("admin");
    httpCfg.secure = config[HTTP_CONTROL]["secure"].value_or(false);
    httpCfg.verify = config[HTTP_CONTROL]["verify"].value_or(true);
    httpCfg.threads = config[HTTP_CONTROL]["threads"].value_or(std::thread::hardware_concurrency());
    auto httpLogger = LoggerFactory::create(readLoggerConfig(config, HTTP_CONTROL));
    HTTPServer httpControlServer(std::move(httpCfg), httpLogger);


    int threads = config[APP]["threads"].value_or(std::thread::hardware_concurrency());
    auto appLogger = LoggerFactory::create(readLoggerConfig(config, APP));
    IRCtoDBConnector connector(threads, appLogger);

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
    auto chLogger = LoggerFactory::create(readLoggerConfig(config, CLICKHOUSE));

    /*connector.initCHConnectionPool(std::move(chCfg), chConns, chLogger);
    if (connector.getCH()->getPoolSize() == 0) {
        appLogger->logCritical("Failed to initialize CHConnectionPool. Exit");
        return 1;
    }*/


    PGConnectionConfig pgCfg;
    pgCfg.host = config[POSTGRESQL]["host"].value_or("localhost");
    pgCfg.port = config[POSTGRESQL]["port"].value_or(5432);
    pgCfg.dbname = config[POSTGRESQL]["dbname"].value_or("postgres");
    pgCfg.user = config[POSTGRESQL]["user"].value_or("postgres");
    pgCfg.pass = config[POSTGRESQL]["password"].value_or("postgres");
    int pgConns = config[POSTGRESQL]["connections"].value_or(std::thread::hardware_concurrency());
    auto pgLogger = LoggerFactory::create(readLoggerConfig(config, POSTGRESQL));

    connector.initPGConnectionPool(std::move(pgCfg), pgConns, pgLogger);
    if (connector.getPG()->getPoolSize() == 0) {
        appLogger->logCritical("Failed to initialize PGConnectionPool. Exit");
        return 1;
    }

    connector.updateChannelsList(connector.loadChannels());

    IRCConnectConfig ircConfig;
    ircConfig.host = config[IRC]["host"].value_or("irc.chat.twitch.tv");
    ircConfig.port = config[IRC]["port"].value_or(6667);

    auto ircLogger = LoggerFactory::create(readLoggerConfig(config, IRC));
    connector.initIRCWorkers(ircConfig, connector.loadAccounts(), ircLogger);

    httpControlServer.addControlUnit(APP, &connector);
    httpControlServer.addControlUnit(CLICKHOUSE, connector.getCH().get());
    httpControlServer.addControlUnit(POSTGRESQL, connector.getPG().get());

    if (!httpControlServer.start(connector.getPool())) {
        appLogger->logCritical("Failed to start HTTP Control Server. Exit");
        return 1;
    }

    // TODO sepparate IRC worker read and write thread
    // read thread moves data to it's hooks(or publish/subscriber)


    // TODO add google language detection, add CH dictionary
    // https://github.com/CLD2Owners/cld2
    // https://github.com/scivey/langdetectpp

    // TODO change CH name and display sizes
    // user max=25

    // TODO provide http interfaces
    //      /shutdown
    //      /reload
    //      /stats
    //      /userlist
    //      /info?user=""
    //      /adduser?user=""&nick=""&password=""
    //      /join?user=""&channel=""
    //      /leave?user=""&channel=""
    // TODO add statistics
    // TODO rewrite message hooks
    // TODO add lua runner or python runner

    IRCtoDBConnector::loop();

    httpControlServer.clearUnits();
}
