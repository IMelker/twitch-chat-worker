//
// Created by l2pic on 22.04.2021.
//

#ifndef CHATCONTROLLER_COMMON_NETWORK_SOCKET_H_
#define CHATCONTROLLER_COMMON_NETWORK_SOCKET_H_

#define HANDLE int
#define INVALID_SOCKET (-1)

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <cstring>

class Socket
{
  public:
    enum Status
    {
        Done,         //!< The socket has sent / received the data
        NotReady,     //!< The socket is not ready to send / receive data yet
        Partial,      //!< The socket sent a part of the data
        Disconnected, //!< The TCP socket has been disconnected
        Error         //!< An unexpected error happened
    };
    enum { AnyPort = 0 };

  protected:
    enum Type { TCP, UDP };
  public:
    explicit Socket(Type type);
    virtual ~Socket() = default;

    void create();
    void create(HANDLE fd);
    void close();

    [[nodiscard]] unsigned short getLocalPort() const;
    [[nodiscard]] HANDLE getHandle() const { return sock; }

    void setBlocking(bool block);
    [[nodiscard]] bool isBlocking() const { return blocking; };

  protected:
    static Status getErrorStatus();
    // Convert a struct sockaddr address to a string, IPv4 and IPv6:
    static std::string getAddress(const struct sockaddr *sa);
    static sockaddr_in createAddress(const std::string &address, int port);
    static sockaddr_in createAddress(unsigned int address, unsigned short port);
  private:
    Type type;
    HANDLE sock = INVALID_SOCKET;
    bool blocking = false;
};

#endif //CHATCONTROLLER_COMMON_NETWORK_SOCKET_H_
