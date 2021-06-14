//
// Created by l2pic on 14.06.2021.
//

#ifndef CHATCONTROLLER__SO5HELPERS_H_
#define CHATCONTROLLER__SO5HELPERS_H_

#include <memory>

#include <so_5/error_logger.hpp>
#include <so_5/msg_tracing.hpp>

#include "Logger.h"

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

#endif //CHATCONTROLLER__SO5HELPERS_H_
