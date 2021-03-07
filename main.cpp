#include "app.h"

#include <cstdio>
#include <unistd.h>
#include <wait.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <thread>

#include <nlohmann/json.hpp>

#include "common/SysSignal.h"
#include "common/Options.h"
#include "common/Config.h"

#include "ircclient/IRCWorker.h"
#include "pg/PGConnectionPool.h"

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

    PGConnectionConfig pgConfig;
    pgConfig.host = config["pg"]["host"].value_or("localhost");
    pgConfig.port = config["pg"]["port"].value_or(5432);
    pgConfig.dbname = config["pg"]["dbname"].value_or("postgres");
    pgConfig.user = config["pg"]["user"].value_or("postgres");
    pgConfig.pass = config["pg"]["password"].value_or("postgres");

    auto pgBackend = std::make_shared<PGConnectionPool>(std::move(pgConfig));

    IRCtoDBConnector connector(pgBackend);

    std::vector<IRCWorker> workers;
    std::string host = config["irc"]["host"].value_or("irc.chat.twitch.tv");
    int port = config["irc"]["port"].value_or(6667);

    std::ifstream credfile(config["irc"]["credentials"].value_or("credentials.json"));
    nlohmann::json credentials;
    credfile >> credentials;

    for (auto &cred : credentials) {
        workers.emplace_back(host, port,
                             cred.at("nick").get<std::string>(),
                             cred.at("user").get<std::string>(),
                             cred.at("password").get<std::string>(),
                             &connector);
    }

}
