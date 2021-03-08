//
// Created by imelker on 08.03.2021.
//

#ifndef CHATSNIFFER__PROCESSOR_H_
#define CHATSNIFFER__PROCESSOR_H_

#include <string>
#include <vector>

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

class PGConnection;
class DataProcessor
{
  public:
    DataProcessor();
    ~DataProcessor();

    void processMessage(const MessageData& msg, PGConnection *conn);
  private:
};

#endif //CHATSNIFFER__PROCESSOR_H_
