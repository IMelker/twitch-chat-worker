//
// Created by l2pic on 08.03.2021.
//

#include "Logger.h"

#include <filesystem>
//#include <iostream>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/hourly_file_sink.h"

inline void createPathIfNotExist(const std::string &filename) {
    std::filesystem::path path(filename);
    if (path.is_relative())
        path = std::filesystem::absolute(path);
    std::filesystem::create_directories(path.parent_path());
}

Logger::Logger() {
    this->logger = spdlog::default_logger();
}

Logger::~Logger() {
    logger->flush();
};

void Logger::setLogLevel(LoggerLevel level) {
    this->logger->set_level(static_cast<spdlog::level::level_enum>(level));
    this->logger->flush();
}

LoggerLevel Logger::getLogLevel() {
    return static_cast<LoggerLevel>(this->logger->level());
}

bool Logger::checkLogLevel(LoggerLevel level) {
    return this->logger->should_log(static_cast<spdlog::level::level_enum>(level));
}

void Logger::setPattern(std::string pattern) {
    this->logger->set_pattern(std::move(pattern));
}

void Logger::setFormatter(std::unique_ptr<spdlog::formatter> formatter) {
    this->logger->set_formatter(std::move(formatter));
}

void Logger::enableBacktrace(size_t number) {
    this->logger->enable_backtrace(number);
}

void Logger::disableBacktrace() {
    this->logger->disable_backtrace();
}

void Logger::dumpBacktrace() {
    this->logger->dump_backtrace();
}

void Logger::flush() {
    this->logger->flush();
}

void Logger::flushOn(LoggerLevel level) {
    this->logger->flush_on(static_cast<spdlog::level::level_enum>(level));
}

void Logger::flushEvery(std::chrono::seconds interval) {
    spdlog::flush_every(interval);
}

ConsoleLogger::ConsoleLogger(const std::string &name) {
    this->logger = spdlog::stdout_color_mt(name);
}

FileLogger::FileLogger(const std::string &name, const std::string &filename) {
    createPathIfNotExist(filename);

    this->logger = spdlog::basic_logger_mt(name, filename);
}

RotateLogger::RotateLogger(const std::string &name, const std::string &filename, int max_size, int max_files) {
    createPathIfNotExist(filename);

    this->logger = spdlog::rotating_logger_mt(name, filename, max_size, max_files, true);
}

DailyLogger::DailyLogger(const std::string &name, const std::string &filename, int hour, int min) {
    createPathIfNotExist(filename);

    this->logger = spdlog::daily_logger_mt(name, filename, hour, min);
}

HourlyLogger::HourlyLogger(const std::string &name, const std::string &filename) {
    createPathIfNotExist(filename);

    this->logger = spdlog::hourly_logger_mt(name, filename);
}

TCPLogger::TCPLogger(const std::string &name, std::string host, int port) {
    spdlog::sinks::tcp_sink_config config{std::move(host), port};
    this->sink = std::make_shared<spdlog::sinks::tcp_sink_mt>(std::move(config));
    this->logger = std::make_shared<spdlog::logger>(name, this->sink);
}

