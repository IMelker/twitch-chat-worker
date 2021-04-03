//
// Created by imelker on 08.03.2021.
//

#include "common/ThreadPool.h"
#include "common/Logger.h"
#include "irc/IRCWorker.h"
#include "Message.h"
#include "MessageProcessor.h"

MessageProcessor::MessageProcessor(ThreadPool *pool, std::shared_ptr<Logger> logger)
  : pool(pool), logger(std::move(logger)) {
    this->logger->logInfo("MessageProcessor init");
    langDetector = langdetectpp::Detector::create();
}

MessageProcessor::~MessageProcessor() {
    logger->logTrace("MessageProcessor end of processor");
}

void MessageProcessor::onMessage(IRCWorker *worker, const IRCMessage &message, long long now) {
    pool->enqueue([message = message, now, worker, this]() {
        Message transformed;

        transform(message, now, transformed);

        logger->logTrace(R"(MessageProcessor process {{worker: "{}", channel: "{}", from: "{}", text: "{}", lang: "{}", valid: {}}})",
                         fmt::ptr(worker), transformed.channel, transformed.user, transformed.text, transformed.lang, transformed.valid);

        dispatch(transformed);
    });
}

void MessageProcessor::transform(const IRCMessage &message, long long now, Message &result) {
    result.channel = message.parameters.at(0)[0] == '#' ? message.parameters.at(0).substr(1) : message.parameters.at(0);
    result.user = message.prefix.nickname;
    result.text = message.parameters.back();
    result.lang = langdetectpp::toShortName(langDetector->detect(result.text));
    result.valid = !result.text.empty();
    result.timestamp = now;
}
