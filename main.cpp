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

#define UNIT_OK 0
#define UNIT_RESTART 1
#define UNIT_STOP -1

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
    options.addOption<bool>("version", "v", "App version", "false");
    options.addOption<bool>("trace-exit-code", "t", "Trace exit code for daemon mode", "false");
    options.parse(argc, argv);

    if (options.getValue<bool>("version")) {
        printf("Name: " APP_NAME "\n"
               "Version: " APP_VERSION "\n"
               "Git: " APP_GIT_HASH "\n"
               "Build: " APP_GIT_DATE " gcc-" __VERSION__ "\n");
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

    auto appLogger = LoggerFactory::create(readLoggerConfig(config, APP));
    DefaultLogger::setAsDefault(appLogger);

    int threads = config[APP]["threads"].value_or(std::thread::hardware_concurrency());
    IRCtoDBConnector connector(threads, appLogger);

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
    auto httpLogger = LoggerFactory::create(readLoggerConfig(config, HTTP_CONTROL));
    HTTPServer httpControlServer(std::move(httpCfg), httpLogger);

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

    connector.initCHConnectionPool(std::move(chCfg), chConns, chLogger);
    if (connector.getCH()->getPoolSize() == 0) {
        appLogger->logCritical("Failed to initialize CHConnectionPool. Exit");
        return UNIT_RESTART;
    }

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
        return UNIT_RESTART;
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
        return UNIT_RESTART;
    }

    // TODO Make http auth for control

    // TODO rewrite message hooks
    // TODO add lua runner or python runner

    // TODO add google language detection, add CH dictionary
    // https://github.com/scivey/langdetectpp

    // TODO change CH name and display sizes
    // user max=25

    // TODO make clickhouse SSL client

    IRCtoDBConnector::loop();
    httpControlServer.clearUnits();

    return UNIT_OK;
}
