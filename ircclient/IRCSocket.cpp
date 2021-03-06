#include <cstring>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "IRCSocket.h"

#define SOCKET_ERROR -1

IRCSocket::IRCSocket() {
    if ((this->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == SOCKET_ERROR) {
        fprintf(stderr, "Socket create failed. Error: %s\n", strerror(errno));
        exit(SOCKET_ERROR);
    }

    int on = 1;
    if (setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, (char const *) &on, sizeof(on)) == SOCKET_ERROR) {
        fprintf(stderr, "Invalid socket. Error: %s\n", strerror(errno));
        exit(SOCKET_ERROR);
    }

    fcntl(this->sock, F_SETFL, O_NONBLOCK);
    fcntl(this->sock, F_SETFL, O_ASYNC);
}

IRCSocket::~IRCSocket() {
    disconnect();
}

bool IRCSocket::connect(char const *host, int port) {
    hostent *he;

    if (!(he = gethostbyname(host))) {
        fprintf(stderr, "Could not resolve host: %s\n", host);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((const in_addr *) he->h_addr);
    memset(&(addr.sin_zero), '\0', 8);

    if (::connect(this->sock, (sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Could not connect to: %s. Error: %s\n", host, strerror(errno));
        return false;
    }

    this->established = true;

    return true;
}

void IRCSocket::disconnect() {
    if (this->established) {
        close(this->sock);
        this->established = false;
    }
}

bool IRCSocket::send(char const *data, size_t len) const {
    if (this->established) {
        if (::send(this->sock, data, len, 0) == SOCKET_ERROR)
            return false;
    }
    return true;
}

int IRCSocket::receive(char *buf, int maxSize) {
    auto bytes = recv(this->sock, buf, maxSize - 1, 0);
    if (bytes > 0) {
        return bytes;
    }
    else if (errno != EINTR)
        disconnect();
    return bytes;
}
