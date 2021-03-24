//
// Created by l2pic on 08.03.2021.
//

#ifndef CHATSNIFFER_COMMON_LOGGER_H_
#define CHATSNIFFER_COMMON_LOGGER_H_

#include <utility>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/tcp_sink.h"
#include "spdlog/fmt/ostr.h"

enum class LoggerLevel : int
{
    Trace = SPDLOG_LEVEL_TRACE,
    Debug = SPDLOG_LEVEL_DEBUG,
    Info = SPDLOG_LEVEL_INFO,
    Warn = SPDLOG_LEVEL_WARN,
    Err = SPDLOG_LEVEL_ERROR,
    Critical = SPDLOG_LEVEL_CRITICAL,
    Off = SPDLOG_LEVEL_OFF
};

std::shared_ptr<spdlog::logger> getLogger(class Logger *logger);

class Logger // Global log
{
  public:
    friend std::shared_ptr<spdlog::logger> getLogger(Logger *logger);

  public:
    Logger();
    virtual ~Logger();

    virtual void setLogLevel(LoggerLevel level);
    virtual LoggerLevel getLogLevel();
    virtual bool checkLogLevel(LoggerLevel level);

    virtual void setPattern(std::string pattern);
    virtual void setFormatter(std::unique_ptr<spdlog::formatter> formatter);

    virtual void enableBacktrace(size_t number);
    virtual void disableBacktrace();
    virtual void dumpBacktrace();

    virtual void flush();
    virtual void flushOn(LoggerLevel level);
    virtual void flushEvery(int sec);
    static void flushEvery(std::chrono::seconds interval);

    template<typename FormatString, typename... Args>
    inline void logTrace(const FormatString &fmt, Args &&...args) {
        this->logger->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename FormatString, typename... Args>
    inline void logDebug(const FormatString &fmt, Args &&...args) {
        this->logger->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename FormatString, typename... Args>
    inline void logInfo(const FormatString &fmt, Args &&...args) {
        this->logger->info(fmt, std::forward<Args>(args)...);
    }

    template<typename FormatString, typename... Args>
    inline void logWarn(const FormatString &fmt, Args &&...args) {
        this->logger->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename FormatString, typename... Args>
    inline void logError(const FormatString &fmt, Args &&...args) {
        this->logger->error(fmt, std::forward<Args>(args)...);
    }

    template<typename FormatString, typename... Args>
    inline void logCritical(const FormatString &fmt, Args &&...args) {
        this->logger->critical(fmt, std::forward<Args>(args)...);
    }

  protected:
    std::shared_ptr<spdlog::logger> logger;
};

inline std::shared_ptr<spdlog::logger> getLogger(Logger *logger) {
    return logger->logger;
}

class ConsoleLogger : public Logger
{
  public:
    explicit ConsoleLogger(const std::string &name);
    ~ConsoleLogger() override = default;
};

class FileLogger : public Logger
{
  public:
    FileLogger(const std::string &name, const std::string &filename);
    ~FileLogger() override = default;
};

class RotateLogger : public Logger
{
  public:
    RotateLogger(const std::string &name, const std::string &filename, int max_size = 1048576 * 300, int max_files = 200);
    ~RotateLogger() override = default;
};

class HourlyLogger : public Logger
{
  public:
    HourlyLogger(const std::string &name, const std::string &filename);
    ~HourlyLogger() override = default;
};

class DailyLogger : public Logger
{
  public:
    DailyLogger(const std::string &name, const std::string &filename, int hour = 0, int min = 0);
    ~DailyLogger() override = default;
};

class MultiLogger : public Logger
{
  public:
    template<typename... Loggers>
    explicit MultiLogger(const std::string &name, Loggers* ...loggers) {
        static_assert(sizeof...(Loggers) == 0, "Empty loggers list is not available for MultiLogger");

        std::vector<std::shared_ptr<spdlog::logger>> raw;
        raw.reserve(sizeof...(Loggers));

        (raw.push_back(getLogger(std::forward<Loggers>(loggers))), ...);

        this->logger = std::make_shared<spdlog::logger>(name, raw.begin(), raw.end());
    }
    ~MultiLogger() override = default;
};

class TCPLogger : public Logger
{
  public:
    TCPLogger(const std::string &name, std::string host, int port);
    ~TCPLogger() override = default;
  private:
    std::shared_ptr<spdlog::sinks::tcp_sink_mt> sink;
};

#endif //CHATSNIFFER_COMMON_LOGGER_H_
