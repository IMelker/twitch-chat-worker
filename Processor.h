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
    long timestamp = 0;
    bool valid = false;
};

class MessageProcessor {
  public:
    MessageProcessor();
    ~MessageProcessor();

    // irc handlers
    void transformMessage(const IRCMessage &message, MessageData &result);
  private:
};

class CHConnectionPool;
class DataProcessor
{
  public:
    DataProcessor();
    ~DataProcessor();

    void processMessage(MessageData &&msg, const std::shared_ptr<CHConnectionPool> &pg);
    void flushMessages(const std::shared_ptr<CHConnectionPool> &pg);
  private:
    std::mutex mutex;
    std::vector<MessageData> batch;
};

#endif //CHATSNIFFER__PROCESSOR_H_
