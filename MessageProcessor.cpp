//
// Created by imelker on 08.03.2021.
//

#include <so_5/send_functions.hpp>

#include "Logger.h"
#include "ThreadName.h"

#include "ChatMessage.h"
#include "MessageProcessor.h"

MessageProcessor::MessageProcessor(const context_t &ctx, so_5::mbox_t listener,
                                   MessageProcessorConfig config, std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), config(std::move(config)), logger(std::move(logger)), listener(std::move(listener)) {
    this->logger->logInfo("MessageProcessor init");

    if (this->config.languageRecognition)
        langDetector = langdetectpp::Detector::create();
}

MessageProcessor::~MessageProcessor() {
    logger->logTrace("MessageProcessor end of processor");
}

void MessageProcessor::so_define_agent() {
    set_thread_name("msg_processor");
    so_subscribe_self().event(&MessageProcessor::evtIrcMessage, so_5::thread_safe);
}

void MessageProcessor::so_evt_start() {

}

void MessageProcessor::so_evt_finish() {

}

void MessageProcessor::evtIrcMessage(const IRCMessage &ircMessage) {
    auto message = transform(ircMessage);

    logger->logTrace(R"(MessageProcessor process: {{uuid: "{}", channel: "{}", from "{}", text: "{}", lang: "{}", valid: {} }})",
                     message->uuid.second, message->channel, message->user, message->text, message->lang ,message->valid);

    so_5::send(listener, message);
}

MessageProcessor::MessageHolder MessageProcessor::transform(const IRCMessage &message) {
    std::string lang = "UNKNOWN";
    if (this->config.languageRecognition)
        lang = langdetectpp::toShortName(langDetector->detect(message.text));

    bool valid = !message.text.empty();

    return MessageHolder::make(message.nickname, message.channel, message.text,
                               std::move(lang), message.timestamp, valid);
}
