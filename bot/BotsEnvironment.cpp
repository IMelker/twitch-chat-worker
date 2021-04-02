//
// Created by imelker on 02.04.2021.
//

#include "../common/Logger.h"
#include "BotsEnvironment.h"

BotsEnvironment::BotsEnvironment(std::shared_ptr<Logger> logger) : logger(std::move(logger)) {

}

BotsEnvironment::~BotsEnvironment() = default;

void BotsEnvironment::onMessage(const Message &message) {
    logger->logTrace("BotsEnvironment: process message from channel {} with text {}", message.channel, message.text);
}

std::tuple<int, std::string> BotsEnvironment::processHttpRequest(std::string_view path,
                                                                 const std::string &request,
                                                                 std::string &error) {
    error = "Failed to match path";
    return EMPTY_HTTP_RESPONSE;
}
