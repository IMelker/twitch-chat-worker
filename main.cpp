#include "app.h"

#include <cstdio>
#include <unistd.h>
#include <wait.h>
#include <thread>

#include <so_5/all.hpp>

#include "SysSignal.h"
#include "Options.h"
#include "Config.h"
#include "ThreadName.h"
#include "LoggerFactory.h"

#include "db/ch/CHConnectionPool.h"
#include "db/pg/PGConnectionPool.h"

#include "HttpController.h"
#include "HttpNotifier.h"
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

struct So5Logger final : public so_5::error_logger_t
{
    explicit So5Logger(std::shared_ptr<Logger> logger) : logger(std::move(logger)) {}
    void log(const char * file_name, unsigned int line, const std::string & message) override {
        logger->logError("[So5Logger] {}:{} {}", file_name, line, message);
    }
  private:
    std::shared_ptr<Logger> logger;
};

struct So5MessageTrace : public so_5::msg_tracing::tracer_t {
  public:
    explicit So5MessageTrace(std::shared_ptr<Logger> logger) : logger(std::move(logger)) {}
    void trace(const std::string &what) noexcept override {
        logger->logWarn("[So5MessageTrace] {}", what);
    }
  private:
    std::shared_ptr<Logger> logger;
};

int main(int argc, char *argv[]) {
    SysSignal::setupSignalHandling();

    Options options{APP_NAME};
    options.addOption<std::string>("config", "c", "Config filepath", "config.toml");
    options.addOption<std::string>("pid-file-name", "p", "Filepath for process ID output", APP_PID_PATH);
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

    DefaultLogger::logInfo(APP_NAME " version: " APP_VERSION " git: "
                           APP_GIT_HASH " build: " APP_GIT_DATE " gcc-" __VERSION__);

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
        set_thread_name("main_launch");
        so_5::launch([&](so_5::environment_t &env) {
                auto httpBox = env.create_mbox();
                env.register_agent_as_coop(env.make_agent<Controller>(httpBox, config, db, appLogger));
                env.register_agent_as_coop(env.make_agent<HttpController>(httpBox, config, httpLogger));
                env.register_agent_as_coop(env.make_agent<HttpNotifier>(config, httpLogger));
            },
            [&]( so_5::environment_params_t & params ) {
                params.error_logger(std::make_shared<So5Logger>(appLogger));
                params.message_delivery_tracer(std::make_unique<So5MessageTrace>(appLogger));

                // Setup filter which enables only messages with null event_handler_data_ptr.
                params.message_delivery_tracer_filter(so_5::msg_tracing::make_filter(
                    [](const so_5::msg_tracing::trace_data_t & td) {
                        const auto h = td.event_handler_data_ptr();
                        return h && (nullptr == *h);
                }));
            });
    }
    catch (const std::exception &e) {
        appLogger->logCritical("Exception: ", e.what());
        return UNIT_RESTART;
    }
    return UNIT_OK;

    // BotEngine
    // TODO change architecture for "precompiled" event handlers and lua executables
    // TODO change architecture with interruptable scripting(courutines)
    // TODO BotEnvironment add http request event
    // TODO BotEnvironment custom global event
    // TODO BotEngine timer events
    // TODO add timer and timer event to bot
    // TODO add statistics for StatsCollector

    // TODO Make bot answers faster
    // 0. TODO add timers for round trip time in chatcontroller
    // 0. TODO fast multithread mbox find for bot
    // 1. TODO remove BotEnvrionment from (MessageProcessor->BotEnvrionment->BotEngine) chain
    // 2. TODO remake ignored for answer nicknames
    // 3. TODO split IRCSelector for read and write threads (?)

    // TODO fix options, remove useless and add controls

    // TODO fix multiple bots on one channel loop

    // Global
    // TODO Review accounts, channels, bots logic
    // TODO add tags support
    // TODO add HTTPClient
    // TODO add TwitchAPI
    // TODO add smileys detection for messages
    // TODO add key value storage

    // TODO make web admin

    // Blockings and users
    // TODO blocked date for bot
    // TODO blocked user for bot

    // Clickhouse
    // TODO set TTL for user_logs messages to 30 days https://clickhouse.tech/docs/en/engines/table-engines/mergetree-family/mergetree/#table_engine-mergetree-ttl
    // TODO make clickhouse SSL client
    // TODO change CH name and display sizes user max=50
}
