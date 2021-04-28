//
// Created by l2pic on 22.04.2021.
//

#ifndef CHATCONTROLLER_COMMON_TCPSOCKET_H_
#define CHATCONTROLLER_COMMON_TCPSOCKET_H_

#include "Socket.h"

class TCPSocket : public Socket
{
  public:
    TCPSocket();
    ~TCPSocket() override;

    [[nodiscard]] std::string getRemoteAddress() const;
    [[nodiscard]] unsigned short getRemotePort() const;

    Socket::Status connect(const std::string &remoteAddress, unsigned short remotePort, std::chrono::milliseconds timeout);
    void disconnect();

    Socket::Status send(const char *data, int size);
    Socket::Status send(const char *data, int size, int &sent);
    Socket::Status receive(char *data, size_t size, int &received);
};

#endif //CHATCONTROLLER_COMMON_TCPSOCKET_H_
