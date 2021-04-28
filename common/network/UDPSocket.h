//
// Created by l2pic on 22.04.2021.
//

#ifndef CHATCONTROLLER_COMMON_UDPSOCKET_H_
#define CHATCONTROLLER_COMMON_UDPSOCKET_H_

#include <string>
#include "Socket.h"

#define MAX_DATAGRAM_SIZE 65507

class UDPSocket : public Socket
{
  public:
    UDPSocket();
    ~UDPSocket() override;

    Socket::Status bind(const std::string &address, unsigned short port);
    void unbind();

    Socket::Status send(const char *data, size_t size,
                        const std::string &remoteAddress, unsigned short remotePort);
    Socket::Status receive(char *data, size_t size, int &received,
                           std::string &remoteAddress, unsigned short &remotePort);
};

#endif //CHATCONTROLLER_COMMON_UDPSOCKET_H_
