//
// Created by imelker on 08.03.2021.
//

#ifndef CHATSNIFFER__MESSAGEPROCESSOR_H_
#define CHATSNIFFER__MESSAGEPROCESSOR_H_

#include <string>
#include <memory>
#include <vector>
#include <set>

#include <langdetectpp/langdetectpp.h>
#include <so_5/agent.hpp>

#include "irc/IRCMessage.h"

#include "ChatMessage.h"

class ThreadPool;
class Logger;

struct MessageProcessorConfig {
    bool languageRecognition = false;
};

class MessageProcessor final : public so_5::agent_t
{
    using MessageHolder = so_5::message_holder_t<Chat::Message>;
  public:
    explicit MessageProcessor(const context_t &ctx,
                              so_5::mbox_t listener,
                              MessageProcessorConfig config,
                              std::shared_ptr<Logger> logger);
    ~MessageProcessor() override;

    // implementation so_5::agent_t
    void so_define_agent() override;
    void so_evt_start() override;
    void so_evt_finish() override;

    // so_5 events
    void evtIrcMessage(const IRCMessage &ircMessage);
  private:
    MessageHolder transform(const IRCMessage &message);

    const MessageProcessorConfig config;

    const std::shared_ptr<Logger> logger;
    std::shared_ptr<langdetectpp::Detector> langDetector;

    so_5::mbox_t listener;
};

#endif //CHATSNIFFER__MESSAGEPROCESSOR_H_
