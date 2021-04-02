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

    auto appLogger = LoggerFactory::create(LoggerFactory::config(config, APP));
    DefaultLogger::setAsDefault(appLogger);

    ThreadPool pool(config[APP]["threads"].value_or(std::thread::hardware_concurrency()));

    Controller controller(config, &pool, appLogger);

    if (!controller.initDBStorage()) {
        appLogger->logCritical("Failed to initialize DBStorage. Exit");
        return UNIT_RESTART;
    }

    // message publisher
    if (!controller.initMessageProcessor()) {
        appLogger->logCritical("Failed to initialize MessageProcessor. Exit");
        return UNIT_RESTART;
    }

    // message subscribers
    if (!controller.initMessageStorage()) {
        appLogger->logCritical("Failed to initialize MessageStorage. Exit");
        return UNIT_RESTART;
    }
    if (!controller.initBotsEnvironment()) {
        appLogger->logCritical("Failed to initialize MessageStorage. Exit");
        return UNIT_RESTART;
    }

    // this will start processing messages
    if (!controller.initIRCWorkerPool()) {
        appLogger->logCritical("Failed to initialize IRCWorkerPool. Exit");
        return UNIT_RESTART;
    }

    if (!controller.startHttpServer()) {
        appLogger->logCritical("Failed to start HTTP Control Server. Exit");
        return UNIT_RESTART;
    }

    // TODO rewrite message hooks
    // TODO add lua runners

    // TODO change CH name and display sizes
    // user max=50

    // TODO add language detection, add CH dictionary
    // https://github.com/scivey/langdetectpp


    // TODO IRCMessage to string_view
    // TODO Make http auth for control
    // TODO make clickhouse SSL client

    Controller::loop();
    return UNIT_OK;
}
