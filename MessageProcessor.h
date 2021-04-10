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

#include "irc/IRCMessage.h"
#include "irc/IRCMessageListener.h"

#include "Message.h"
#include "MessagePublisher.h"

class IRCWorker;
class ThreadPool;
class Logger;

struct MessageProcessorConfig {
    bool languageRecognition = false;
};

class MessageProcessor : public IRCMessageListener,
                         public MessagePublisher
{
  public:
    explicit MessageProcessor(MessageProcessorConfig config,
                              ThreadPool *pool,
                              std::shared_ptr<Logger> logger);
    ~MessageProcessor() override;

    // implementation IRCMessageProxy
    void onMessage(IRCWorker *worker, const IRCMessage &message, long long now) override;

    std::shared_ptr<Message> transform(const IRCMessage &message, long long now);
  private:
    MessageProcessorConfig config;

    ThreadPool *pool;
    std::shared_ptr<Logger> logger;
    std::shared_ptr<langdetectpp::Detector> langDetector;
};

#endif //CHATSNIFFER__MESSAGEPROCESSOR_H_
