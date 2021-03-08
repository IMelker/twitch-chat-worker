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
#include "pgsql/PGConnectionPool.h"

#include "IRCtoDBConnector.h"


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

    LoggerConfig appLogConfig;
    appLogConfig.name = "app";
    appLogConfig.type = LoggerFactory::typeFromString(config["general"]["log_type"].value_or("file"));
    appLogConfig.level = LoggerFactory::levelFromString(config["general"]["log_level"].value_or("info"));
    appLogConfig.target = config["general"]["log_target"].value_or("app.log");

    int threads = config["pg"]["threads"].value_or(std::thread::hardware_concurrency());
    IRCtoDBConnector connector(threads, LoggerFactory::create(appLogConfig));

    PGConnectionConfig pgConfig;
    pgConfig.host = config["pg"]["host"].value_or("localhost");
    pgConfig.port = config["pg"]["port"].value_or(5432);
    pgConfig.dbname = config["pg"]["dbname"].value_or("postgres");
    pgConfig.user = config["pg"]["user"].value_or("postgres");
    pgConfig.pass = config["pg"]["password"].value_or("postgres");

    LoggerConfig pgLogConfig;
    pgLogConfig.name = "sql";
    pgLogConfig.type = LoggerFactory::typeFromString(config["pg"]["log_type"].value_or("file"));
    pgLogConfig.level = LoggerFactory::levelFromString(config["pg"]["log_level"].value_or("info"));
    pgLogConfig.target = config["pg"]["log_target"].value_or("pg.log");

    int connections = config["pg"]["connections"].value_or(std::thread::hardware_concurrency());
    connector.initPGConnectionPool(std::move(pgConfig), connections, LoggerFactory::create(pgLogConfig));

    IRCConnectConfig ircConfig;
    ircConfig.host = config["irc"]["host"].value_or("irc.chat.twitch.tv");
    ircConfig.port = config["irc"]["port"].value_or(6667);

    LoggerConfig ircLogConfig;
    ircLogConfig.name = "irc";
    ircLogConfig.type = LoggerFactory::typeFromString(config["irc"]["log_type"].value_or("file"));
    ircLogConfig.level = LoggerFactory::levelFromString(config["irc"]["log_level"].value_or("info"));
    ircLogConfig.target = config["irc"]["log_target"].value_or("irc.log");

    auto credentials = config["irc"]["credentials"].value_or("credentials.json");
    connector.initIRCWorkers(ircConfig, credentials, LoggerFactory::create(ircLogConfig));

    // TODO add message batcher
    // TODO add caches for channels
    // TODO add statistics
    // TODO add HTTP interface
    //      /shutdown
    //      /reload
    //      /stats
    //      /userlist
    //      /info?user=""
    //      /adduser?user=""&nick=""&password=""
    //      /join?user=""&channel=""
    //      /leave?user=""&channel=""

    IRCtoDBConnector::loop();
}
