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

#include "ircclient/IRCWorker.h"
#include "db/ch/CHConnectionPool.h"
#include "db/pg/PGConnectionPool.h"

#include "IRCtoDBConnector.h"

inline LoggerConfig readLoggerConfig(Config& config, const std::string& category) {
    LoggerConfig logConfig;
    logConfig.name = category;
    logConfig.type = LoggerFactory::typeFromString(config[category]["log_type"].value_or("file"));
    logConfig.level = LoggerFactory::levelFromString(config[category]["log_level"].value_or("info"));
    logConfig.target = config[category]["log_target"].value_or(category + ".log");
    return logConfig;
}

int main(int argc, char *argv[]) {
    SysSignal::setupSignalHandling();

    Options options{APP_NAME};
    options.addOption<std::string>("config", "c", "Config filepath", "config.toml");
    options.addOption<std::string>("pid-file-name", "p", "Filepath for process ID output", "/var/run/dbgateway.pid");
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

    int threads = config["app"]["threads"].value_or(std::thread::hardware_concurrency());
    IRCtoDBConnector connector(threads, LoggerFactory::create(readLoggerConfig(config, "app")));

    CHConnectionConfig chCfg;
    chCfg.host = config["clickhouse"]["host"].value_or("localhost");
    chCfg.port = config["clickhouse"]["port"].value_or(5432);
    chCfg.dbname = config["clickhouse"]["dbname"].value_or("postgres");
    chCfg.user = config["clickhouse"]["user"].value_or("postgres");
    chCfg.pass = config["clickhouse"]["password"].value_or("postgres");
    chCfg.secure = config["clickhouse"]["secure"].value_or(false);
    chCfg.verify = config["clickhouse"]["verify"].value_or(true);
    int chConns = config["clickhouse"]["connections"].value_or(std::thread::hardware_concurrency());
    connector.initCHConnectionPool(std::move(chCfg), chConns, LoggerFactory::create(readLoggerConfig(config, "ch")));

    PGConnectionConfig pgCfg;
    pgCfg.host = config["pg"]["host"].value_or("localhost");
    pgCfg.port = config["pg"]["port"].value_or(5432);
    pgCfg.dbname = config["pg"]["dbname"].value_or("postgres");
    pgCfg.user = config["pg"]["user"].value_or("postgres");
    pgCfg.pass = config["pg"]["password"].value_or("postgres");
    int pgConns = config["pg"]["connections"].value_or(std::thread::hardware_concurrency());
    connector.initPGConnectionPool(std::move(pgCfg), pgConns, LoggerFactory::create(readLoggerConfig(config, "pg")));

    IRCConnectConfig ircConfig;
    ircConfig.host = config["irc"]["host"].value_or("irc.chat.twitch.tv");
    ircConfig.port = config["irc"]["port"].value_or(6667);

    auto credentials = config["irc"]["credentials"].value_or("credentials.json");
    connector.initIRCWorkers(ircConfig, credentials, LoggerFactory::create(readLoggerConfig(config, "irc")));

    // TODO add statistics
    // TODO add HTTP interface, may be based on boost
    // TODO provide http interfaces
    //      /shutdown
    //      /reload
    //      /stats
    //      /userlist
    //      /info?user=""
    //      /adduser?user=""&nick=""&password=""
    //      /join?user=""&channel=""
    //      /leave?user=""&channel=""
    // TODO rewrite message hooks
    // TODO add lua runner or python runner

    IRCtoDBConnector::loop();
}
