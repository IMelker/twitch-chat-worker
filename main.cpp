#include "app.h"

#include <cstdio>
#include <unistd.h>
#include <wait.h>
#include <thread>

#include "common/SysSignal.h"
#include "common/Options.h"
#include "common/Config.h"
#include "common/LoggerFactory.h"

#include "irc/IRCWorker.h"
#include "db/ch/CHConnectionPool.h"
#include "db/pg/PGConnectionPool.h"

#include "Controller.h"

#define UNIT_OK 0
#define UNIT_RESTART 1
#define UNIT_STOP -1

void printVersion() {
    printf("Name: " APP_NAME "\nVersion: " APP_VERSION "\nAuthor: " APP_AUTHOR_EMAIL "\n"
           "Git: " APP_GIT_HASH "\nBuild: " APP_GIT_DATE " gcc-" __VERSION__ "\n");
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

    auto logger = LoggerFactory::create(LoggerFactory::config(config, APP));
    DefaultLogger::setAsDefault(logger);

    ThreadPool pool(config[APP]["threads"].value_or(std::thread::hardware_concurrency()));
    Controller controller(config, &pool, logger);

    // init DBController
    if (!controller.initDBController()) {
        logger->logCritical("Failed to initialize DBController. Exit");
        return UNIT_RESTART;
    }

    if (!controller.initMessageProcessor()) {
        logger->logCritical("Failed to initialize MessageProcessor. Exit");
        return UNIT_RESTART;
    }

    if (!controller.initMessageStorage()) {
        logger->logCritical("Failed to initialize MessageStorage. Exit");
        return UNIT_RESTART;
    }

    if (!controller.initBotsEnvironment()) {
        logger->logCritical("Failed to initialize MessageStorage. Exit");
        return UNIT_RESTART;
    }

    // this will create connections with logins
    if (!controller.initIRCWorkerPool()) {
        logger->logCritical("Failed to initialize IRCWorkerPool. Exit");
        return UNIT_RESTART;
    }

    if (!controller.startBotsEnvironment()) {
        logger->logCritical("Failed to start Bots. Exit");
        return UNIT_RESTART;
    }

    // Start HTTPServer as last instance
    if (!controller.startHttpServer()) {
        logger->logCritical("Failed to start HTTP Control Server. Exit");
        return UNIT_RESTART;
    }


    // TODO move all to SObjectizer framework
    // TODO add remove timer events from Timer
    // TODO add timer and timer event to bot

    // TODO add bot, accounts update

    // TODO Review accounts, channels, bots logic
    // TODO TCP selector for socket for multiple sockets
    // TODO add tags support
    // TODO add HTTPClient
    // TODO add TwitchAPI
    // TODO add smileys detection for messages

    // TODO blocked date for bot
    // TODO blocked user for bot

    // TODO IRCMessage to string_view
    // TODO Make http auth for control
    // TODO make clickhouse SSL client
    // TODO change CH name and display sizes user max=50

    Controller::loop();
    return UNIT_OK;
}
