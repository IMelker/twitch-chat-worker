//
// Created by l2pic on 23.04.2021.
//

#ifndef CHATCONTROLLER_COMMON_NETWORK_TCPLISTENER_H_
#define CHATCONTROLLER_COMMON_NETWORK_TCPLISTENER_H_

#include <string>
#include "Socket.h"
#include "TCPSocket.h"

class TCPListener : public Socket
{
  public:
    TCPListener();
    ~TCPListener() override;

    Socket::Status listen(const std::string &address, unsigned short port);
    Socket::Status accept(TCPSocket &socket);
};

#endif //CHATCONTROLLER_COMMON_NETWORK_TCPLISTENER_H_
