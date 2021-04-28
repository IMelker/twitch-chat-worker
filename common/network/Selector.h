//
// Created by l2pic on 22.04.2021.
//

#ifndef CHATCONTROLLER_COMMON_SELECTOR_H_
#define CHATCONTROLLER_COMMON_SELECTOR_H_

#include <algorithm>
#include <chrono>
#include "Socket.h"

// Multiplexer that allows to read and write to multiple sockets.
class Selector
{
  public:
    Selector();
    ~Selector() = default;

    void add(Socket &socket);
    void remove(Socket &socket);
    void clear();

    // Wait until one or more sockets are ready to receive
    bool wait(std::chrono::milliseconds timeout = std::chrono::seconds(0));

    // Test a socket to know if it is ready to receive data
    bool isReadyForRead(Socket &socket) const;
    // Test a socket to know if it is ready to send data
    bool isReadyForWrite(Socket &socket) const;
  private:
    fd_set allSockets;
    fd_set socketsReadyForRead;
    fd_set socketsReadyForWrite;
    int maxSocket;
    int socketCount;
};

#endif //CHATCONTROLLER_COMMON_SELECTOR_H_
