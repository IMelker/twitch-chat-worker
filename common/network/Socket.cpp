//
// Created by l2pic on 22.04.2021.
//

#include "../Logger.h"
#include "Socket.h"

Socket::Socket(Socket::Type type) : type(type) {

}

unsigned short Socket::getLocalPort() const {
    if (getHandle() != INVALID_SOCKET) {
        // Retrieve informations about the local end of the socket
        sockaddr_in address{};
        socklen_t size = sizeof(address);
        if (getsockname(getHandle(), reinterpret_cast<sockaddr *>(&address), &size) != -1) {
            return ntohs(address.sin_port);
        }
    }
    // We failed to retrieve the port
    return 0;
}

void Socket::create() {
    // Don't create the socket if it already exists
    if (sock == INVALID_SOCKET) {
        HANDLE fd = socket(PF_INET, type == TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (fd == INVALID_SOCKET) {
            DefaultLogger::logError("Failed to create socket: {} - {}", errno, strerror(errno));
            return;
        }

        create(fd);
    }
}

void Socket::create(HANDLE fd) {
    if (sock == INVALID_SOCKET) {
        // Assign the new handle
        sock = fd;

        setBlocking(blocking);

        if (type == TCP) {
            // Disable the Nagle algorithm (i.e. removes buffering of TCP packets)
            int yes = 1;
            if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&yes), sizeof(yes)) == -1) {
                DefaultLogger::logError("Failed to set socket option \"TCP_NODELAY\" ; "
                                        "all your TCP packets will be buffered");
            }
        } else {
            // Enable broadcast by default for UDP sockets
            int yes = 1;
            if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char *>(&yes), sizeof(yes)) == -1) {
                DefaultLogger::logError("Failed to enable broadcast on UDP socket");
            }
        }
    }
}

void Socket::close() {
    // Close the socket
    if (sock == INVALID_SOCKET) {
        ::close(sock);
        sock = INVALID_SOCKET;
    }
}

Socket::Status Socket::getErrorStatus() {
    // The followings are sometimes equal to EWOULDBLOCK,
    // so we have to make a special case for them in order
    // to avoid having double values in the switch case
    if ((errno == EAGAIN) || (errno == EINPROGRESS))
        return NotReady;

    switch (errno)
    {
        case EWOULDBLOCK:  return NotReady;
        case ECONNABORTED: return Disconnected;
        case ECONNRESET:   return Disconnected;
        case ETIMEDOUT:    return Disconnected;
        case ENETRESET:    return Disconnected;
        case ENOTCONN:     return Disconnected;
        case EPIPE:        return Disconnected;
        default:           return Error;
    }
}

void Socket::setBlocking(bool block) {
    // Apply if the socket is already created
    if (sock != INVALID_SOCKET) {
        int status = fcntl(sock, F_GETFL);
        if (block) {
            if (fcntl(sock, F_SETFL, status & ~O_NONBLOCK) == -1)
                DefaultLogger::logError("Failed to set file status flags: {} - {}", errno, strerror(errno));
        } else {
            if (fcntl(sock, F_SETFL, status | O_NONBLOCK) == -1)
                DefaultLogger::logError("Failed to set file status flags: {} - {}", errno, strerror(errno));

        }
    }

    blocking = block;
}

sockaddr_in Socket::createAddress(const std::string &address, int port) {
    hostent *he;
    if (!(he = gethostbyname(address.c_str()))) {
        DefaultLogger::logError("Could not resolve host: {}", address);
        return sockaddr_in{};
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((const in_addr *) he->h_addr);
    memset(&(addr.sin_zero), '\0', 8);
    return addr;
}

sockaddr_in Socket::createAddress(unsigned int address, unsigned short port) {
    sockaddr_in addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = htonl(address);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    return addr;
}

std::string Socket::getAddress(const struct sockaddr *sa) {
    std::string res(28, '\0');
    switch (sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET,
                      &(((struct sockaddr_in *) sa)->sin_addr),
                      const_cast<char *>(res.data()),
                      res.size());
            break;
        case AF_INET6:
            inet_ntop(AF_INET6,
                      &(((struct sockaddr_in6 *) sa)->sin6_addr),
                      const_cast<char *>(res.data()),
                      res.size());
            break;
        default:res.clear();
    }
    return res;
}
