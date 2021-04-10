//
// Created by imelker on 08.03.2021.
//

#include "common/ThreadPool.h"
#include "common/Logger.h"
#include "irc/IRCWorker.h"
#include "Message.h"
#include "MessageProcessor.h"

MessageProcessor::MessageProcessor(MessageProcessorConfig config,
                                   ThreadPool *pool,
                                   std::shared_ptr<Logger> logger)
  : config(std::move(config)), pool(pool), logger(std::move(logger)) {

    this->logger->logInfo("MessageProcessor init");

    if (this->config.languageRecognition)
        langDetector = langdetectpp::Detector::create();
}

MessageProcessor::~MessageProcessor() {
    logger->logTrace("MessageProcessor end of processor");
}

void MessageProcessor::onMessage(IRCWorker *worker, const IRCMessage &message, long long now) {
    pool->enqueue([ircMessage = message, now, worker, this]() {
        auto message = transform(ircMessage, now);

        logger->logTrace(R"(MessageProcessor process {{worker: "{}", channel: "{}", from: "{}", text: "{}", lang: "{}", valid: {}}})",
                         fmt::ptr(worker), message->channel, message->user, message->text, message->lang, message->valid);

        dispatch(message);
    });
}

std::shared_ptr<Message> MessageProcessor::transform(const IRCMessage &message, long long now) {
    std::string channel = message.parameters.at(0)[0] == '#' ? message.parameters.at(0).substr(1) : message.parameters.at(0);
    std::string text = message.parameters.back();

    std::string lang = "UNKNOWN";
    if (this->config.languageRecognition)
        lang = langdetectpp::toShortName(langDetector->detect(text));

    bool valid = !text.empty();

    return std::make_shared<Message>(message.prefix.nickname, std::move(channel),
                                     std::move(text), std::move(lang), now, valid);
}
