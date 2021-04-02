//
// Created by l2pic on 08.03.2021.
//

#ifndef CHATSNIFFER_COMMON_LOGGERFACTORY_H_
#define CHATSNIFFER_COMMON_LOGGERFACTORY_H_

#include "Utils.h"
#include "Config.h"
#include "Logger.h"

enum class LoggerType {
    Default, Console, File, Rotate, Hourly, Daily, Tcp
};

struct LoggerConfig {
    std::string name;
    LoggerType type = LoggerType::Default;
    LoggerLevel level = LoggerLevel::Info;
    std::string target; // filename or host:port
    LoggerLevel flushOn = LoggerLevel::Off;
    int flushEvery = 0;
};

inline std::tuple<std::string, int> parseHostname(const std::string& hostname) {
    int dotPos = hostname.find(':');
    return {hostname.substr(0, dotPos), std::atoi(hostname.substr(dotPos).c_str())};
}

struct LoggerFactory
{
    static LoggerType typeFromString(std::string type) {
        Utils::String::toLower(&type);

        if (type == "file") {
            return LoggerType::File;
        } else if (type == "console") {
            return LoggerType::Console;
        } else if (type == "rotate") {
            return LoggerType::Rotate;
        } else if (type == "hourly") {
            return LoggerType::Hourly;
        } else if (type == "daily") {
            return LoggerType::Daily;
        } else if (type == "tcp") {
            return LoggerType::Tcp;
        } else {
            return LoggerType::Default;
        }
    };

    static LoggerLevel levelFromString(std::string level) {
        Utils::String::toLower(&level);

        if (level == "trace") {
            return LoggerLevel::Trace;
        } else if (level == "debug") {
            return LoggerLevel::Debug;
        } else if (level == "info") {
            return LoggerLevel::Info;
        } else if (level == "warning") {
            return LoggerLevel::Warn;
        } else if (level == "error") {
            return LoggerLevel::Err;
        } else if (level == "critical") {
            return LoggerLevel::Critical;
        } else {
            return LoggerLevel::Off;
        }
    }

    static std::shared_ptr<Logger> create(const LoggerConfig& config) {
        std::shared_ptr<Logger> logger;
        switch (config.type) {
            case LoggerType::Console:
                logger = std::make_shared<ConsoleLogger>(config.name);
                break;
            case LoggerType::File:
                logger = std::make_shared<FileLogger>(config.name, config.target);
                break;
            case LoggerType::Rotate:
                logger = std::make_shared<RotateLogger>(config.name, config.target);
                break;
            case LoggerType::Hourly:
                logger = std::make_shared<HourlyLogger>(config.name, config.target);
                break;
            case LoggerType::Daily:
                logger = std::make_shared<DailyLogger>(config.name, config.target);
                break;
            case LoggerType::Tcp:
                {
                    auto [host, port] = parseHostname(config.target);
                    logger = std::make_shared<DailyLogger>(config.name, host, port);
                    break;
                }
            case LoggerType::Default:
            default:
                logger = std::make_shared<Logger>();
                break;
        }

        logger->setLogLevel(config.level);

        if(config.flushOn != LoggerLevel::Off)
            logger->flushOn(config.flushOn);

        if (config.flushEvery > 0)
            logger->flushEvery(config.flushEvery);

        return logger;
    };

    static LoggerConfig config(Config& config, const std::string& category) {
        LoggerConfig logConfig;
        logConfig.name = category;
        logConfig.type = LoggerFactory::typeFromString(config[category]["log_type"].value_or("file"));
        logConfig.level = LoggerFactory::levelFromString(config[category]["log_level"].value_or("info"));
        logConfig.target = config[category]["log_target"].value_or(category + ".log");
        logConfig.flushEvery = config[category]["flush_every"].value_or(10);
        logConfig.flushOn = LoggerFactory::levelFromString(config[category]["flush_on"].value_or("error"));
        return logConfig;
    }
};


#endif //CHATSNIFFER_COMMON_LOGGERFACTORY_H_
