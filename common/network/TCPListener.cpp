//
// Created by l2pic on 23.04.2021.
//

#include "../Logger.h"
#include "TCPListener.h"

TCPListener::TCPListener() : Socket(TCP) {

}

TCPListener::~TCPListener() = default;

Socket::Status TCPListener::listen(const std::string &address, unsigned short port) {
    // Close the socket if it is already bound
    close();

    // Create the internal socket if it doesn't exist
    create();

    // Check if the address is valid
    if ((address.empty()))
        return Error;

    // Bind the socket to the specified port
    sockaddr_in addr = Socket::createAddress(address, port);
    if (bind(getHandle(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        // Not likely to happen, but...
        DefaultLogger::logError("Failed to bind listener socket to port {}", port);
        return Error;
    }

    // Listen to the bound port
    if (::listen(getHandle(), SOMAXCONN) == -1) {
        // Oops, socket is deaf
        DefaultLogger::logError("Failed to listen to port {}", port);
        return Error;
    }

    return Done;
}

Socket::Status TCPListener::accept(TCPSocket &socket) {
    // Make sure that we're listening
    if (getHandle() == INVALID_SOCKET) {
        DefaultLogger::logError("Failed to accept a new connection, the socket is not listening");
        return Error;
    }

    // Accept a new connection
    sockaddr_in address{};
    socklen_t length = sizeof(address);
    HANDLE remote = ::accept(getHandle(), reinterpret_cast<sockaddr *>(&address), &length);

    // Check for errors
    if (remote == INVALID_SOCKET)
        return Socket::getErrorStatus();

    // Initialize the new connected socket
    socket.close();
    socket.create(remote);

    return Done;
}
