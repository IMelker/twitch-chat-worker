//
// Created by l2pic on 01.04.2021.
//

#ifndef CHATSNIFFER__MESSAGESUBSCRIBER_H_
#define CHATSNIFFER__MESSAGESUBSCRIBER_H_

#include "Message.h"

struct MessageSubscriber
{
    virtual void onMessage(const Message &message) = 0;
};

#endif //CHATSNIFFER__MESSAGESUBSCRIBER_H_
