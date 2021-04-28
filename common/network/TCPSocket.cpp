//
// Created by l2pic on 22.04.2021.
//

#include "../Logger.h"
#include "TCPSocket.h"

TCPSocket::TCPSocket() : Socket(TCP) {

}

TCPSocket::~TCPSocket() = default;

std::string TCPSocket::getRemoteAddress() const {
    if (getHandle() != INVALID_SOCKET) {
        // Retrieve informations about the remote end of the socket
        sockaddr_in address{};
        socklen_t size = sizeof(address);
        if (getpeername(getHandle(), reinterpret_cast<sockaddr *>(&address), &size) != -1) {
            return Socket::getAddress(reinterpret_cast<sockaddr *>(&address));
        }
    }
    // We failed to retrieve the address
    return "";
}

unsigned short TCPSocket::getRemotePort() const {
    if (getHandle() != INVALID_SOCKET) {
        // Retrieve informations about the remote end of the socket
        sockaddr_in address{};
        socklen_t size = sizeof(address);
        if (getpeername(getHandle(), reinterpret_cast<sockaddr *>(&address), &size) != -1) {
            return ntohs(address.sin_port);
        }
    }

    // We failed to retrieve the port
    return 0;
}

Socket::Status TCPSocket::connect(const std::string &remoteAddress, unsigned short remotePort,
                                  std::chrono::milliseconds timeout) {
    // Disconnect the socket if it is already connected
    disconnect();

    // Create the internal socket if it doesn't exist
    create();

    // Create the remote address
    sockaddr_in address = Socket::createAddress(remoteAddress, remotePort);

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
    if (ms.count() <= 0) {
        // ----- We're not using a timeout: just try to connect -----

        // Connect the socket
        if (::connect(getHandle(), reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1)
            return Socket::getErrorStatus();

        // Connection succeeded
        return Done;
    } else {
        // ----- We're using a timeout: we'll need a few tricks to make it work -----

        // Save the previous blocking state
        bool blocking = isBlocking();

        // Switch to non-blocking to enable our connection timeout
        if (blocking)
            setBlocking(false);

        // Try to connect to the remote address
        if (::connect(getHandle(), reinterpret_cast<sockaddr *>(&address), sizeof(address)) >= 0) {
            // We got instantly connected! (it may no happen a lot...)
            setBlocking(blocking);
            return Done;
        }

        // Get the error status
        Status status = Socket::getErrorStatus();

        // If we were in non-blocking mode, return immediately
        if (!blocking)
            return status;

        // Otherwise, wait until something happens to our socket (success, timeout or error)
        if (status == Socket::NotReady) {
            // Setup the selector
            fd_set selector;
            FD_ZERO(&selector);
            FD_SET(getHandle(), &selector);

            // Setup the timeout
            struct timeval time{};
            time.tv_sec = ms.count() / 1000;
            time.tv_usec = (ms.count() % 1000) * 1000;

            // Wait for something to write on our socket (which means that the connection request has returned)
            if (select(static_cast<int>(getHandle() + 1), nullptr, &selector, nullptr, &time) > 0) {
                // At this point the connection may have been either accepted or refused.
                // To know whether it's a success or a failure, we must check the address of the connected peer
                if (!getRemoteAddress().empty()) {
                    // Connection accepted
                    status = Done;
                } else {
                    // Connection refused
                    status = Socket::getErrorStatus();
                }
            } else {
                // Failed to connect before timeout is over
                status = Socket::getErrorStatus();
            }
        }

        // Switch back to blocking mode
        setBlocking(true);

        return status;
    }
}

void TCPSocket::disconnect() {
    // Close the socket
    close();
}

Socket::Status TCPSocket::send(const char *data, int size) {
    if (!isBlocking())
        DefaultLogger::logWarn("Partial sends might not be handled properly.");

    int sent;
    return send(data, size, sent);
}

Socket::Status TCPSocket::send(const char *data, int size, int &sent) {
    // Check the parameters
    if (!data || (size == 0)) {
        DefaultLogger::logError("Cannot send data over the network (no data to send)");
        return Error;
    }

    // Loop until every byte has been sent
    int result = 0;
    for (sent = 0; sent < size; sent += result) {
        // Send a chunk of data
        result = ::send(getHandle(), data + sent, size - sent, 0);

        // Check for errors
        if (result < 0) {
            Status status = Socket::getErrorStatus();

            if ((status == NotReady) && sent)
                return Partial;

            return status;
        }
    }

    return Done;
}

Socket::Status TCPSocket::receive(char *data, size_t size, int &received) {
    // First clear the variables to fill
    received = 0;

    // Check the destination buffer
    if (!data) {
        DefaultLogger::logError("Cannot receive data from the network (the destination buffer is invalid)");
        return Error;
    }

    // Receive a chunk of bytes
    int sizeReceived = recv(getHandle(), data, size, 0);

    // Check the number of bytes received
    if (sizeReceived > 0) {
        received = sizeReceived;
        return Done;
    } else if (sizeReceived == 0) {
        return Socket::Disconnected;
    } else {
        return Socket::getErrorStatus();
    }
}

