//
// Created by l2pic on 22.04.2021.
//

#include "../Logger.h"
#include "UDPSocket.h"

UDPSocket::UDPSocket() : Socket(UDP) {

}
UDPSocket::~UDPSocket() = default;

Socket::Status UDPSocket::bind(const std::string &address, unsigned short port) {
    // Close the socket if it is already bound
    close();

    // Create the internal socket if it doesn't exist
    create();

    // Check if the address is valid
    if (address.empty())
        return Error;

    sockaddr_in addr = Socket::createAddress(address, port);

    // Bind the socket
    if (::bind(getHandle(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        DefaultLogger::logError("Failed to bind socket to address {}:{}", address, port);
        return Error;
    }

    return Done;
}

void UDPSocket::unbind() {
    // Simply close the socket
    close();
}

Socket::Status UDPSocket::send(const char *data, size_t size, const std::string &remoteAddress, unsigned short remotePort) {
    // Create the internal socket if it doesn't exist
    create();

    // Make sure that all the data will fit in one datagram
    if (size > MAX_DATAGRAM_SIZE) {
        DefaultLogger::logError("Cannot send data over the network "
                                "(the number of bytes to send is greater than MAX_DATAGRAM_SIZE)");
        return Error;
    }

    // Build the target address
    sockaddr_in address = Socket::createAddress(remoteAddress, remotePort);

    // Send the data (unlike TCP, all the data is always sent in one call)
    int sent = sendto(getHandle(), data, size, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address));

    // Check for errors
    if (sent < 0)
        return Socket::getErrorStatus();

    return Done;
}

Socket::Status UDPSocket::receive(char *data, size_t size, int &received,
                                  std::string &remoteAddress, unsigned short &remotePort) {
    // First clear the variables to fill
    received = 0;
    remoteAddress.clear();
    remotePort = 0;

    // Check the destination buffer
    if (!data) {
        DefaultLogger::logError("Cannot receive data from the network (the destination buffer is invalid)");
        return Error;
    }

    // Data that will be filled with the other computer's address
    sockaddr_in address = Socket::createAddress(INADDR_ANY, 0);

    // Receive a chunk of bytes
    socklen_t addressSize = sizeof(address);
    int sizeReceived = recvfrom(getHandle(), data, size, 0, reinterpret_cast<sockaddr *>(&address), &addressSize);

    // Check for errors
    if (sizeReceived < 0)
        return Socket::getErrorStatus();

    // Fill the sender information
    received = sizeReceived;
    remoteAddress = Socket::getAddress(reinterpret_cast<const sockaddr *>(&address));
    remotePort = ntohs(address.sin_port);

    return Done;
}
