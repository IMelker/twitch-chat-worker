//
// Created by imelker on 08.03.2021.
//

#ifndef CHATSNIFFER__PROCESSOR_H_
#define CHATSNIFFER__PROCESSOR_H_

#include <string>
#include <vector>
#include <set>

#include "ircclient/IRCClient.h"

struct MessageData {
    std::string user;
    std::string channel;
    std::string text;
    std::string lang;
    long long timestamp = 0;
    bool valid = false;
};

namespace langdetectpp {
class Detector;
}

class MessageProcessor {
  public:
    MessageProcessor();
    ~MessageProcessor();

    // irc handlers
    void transformMessage(const IRCMessage &message, MessageData &result);
  private:
    std::shared_ptr<langdetectpp::Detector> langDetector;
};

class CHConnectionPool;
class DataProcessor
{
  public:
    DataProcessor();
    ~DataProcessor();

    void processMessage(MessageData &&msg, std::shared_ptr<CHConnectionPool> ch);
    void flushMessages(std::shared_ptr<CHConnectionPool> ch);
  private:
    std::mutex mutex;
    std::vector<MessageData> batch;
};

#endif //CHATSNIFFER__PROCESSOR_H_
