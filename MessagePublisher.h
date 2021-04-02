//
// Created by imelker on 01.04.2021.
//

#ifndef CHATSNIFFER__MESSAGEPUBLISHER_H_
#define CHATSNIFFER__MESSAGEPUBLISHER_H_

#include <mutex>
#include <vector>
#include <algorithm>
#include "Message.h"
#include "MessageSubscriber.h"

class MessagePublisher
{
  public:
    MessagePublisher() = default;
    virtual ~MessagePublisher() = default;

    void subscribe(MessageSubscriber* subscriber) {
        subscribers.push_back(subscriber);
    };
    void unsubscribe(MessageSubscriber* subscriber) {
        subscribers.erase(std::find(subscribers.begin(), subscribers.end(), subscriber));
    }

    void dispatch(const Message &message) {
        for (auto *subscriber: subscribers) {
            subscriber->onMessage(message);
        }
    }
  private:
    std::vector<MessageSubscriber*> subscribers;
};

#endif //CHATSNIFFER__MESSAGEPUBLISHER_H_
