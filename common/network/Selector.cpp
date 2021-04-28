//
// Created by l2pic on 22.04.2021.
//

#include "../Logger.h"
#include "Selector.h"

Selector::Selector() {
    clear();
}

void Selector::add(Socket &socket) {
    HANDLE handle = socket.getHandle();
    if (handle != INVALID_SOCKET) {
        if (handle >= FD_SETSIZE) {
            DefaultLogger::logError("The socket can't be added to the selector because its "
                                    "ID is too high. This is a limitation of your operating "
                                    "system's FD_SETSIZE setting.");
            return;
        }

        // SocketHandle is an int in POSIX
        maxSocket = std::max(maxSocket, handle);

        FD_SET(handle, &allSockets);
    }
}

void Selector::remove(Socket &socket) {
    HANDLE handle = socket.getHandle();
    if (handle != INVALID_SOCKET) {
        FD_CLR(handle, &allSockets);
        FD_CLR(handle, &socketsReadyForRead);
    }
}

void Selector::clear() {
    FD_ZERO(&allSockets);
    FD_ZERO(&socketsReadyForRead);
    FD_ZERO(&socketsReadyForWrite);

    maxSocket = 0;
    socketCount = 0;
}

bool Selector::wait(std::chrono::milliseconds timeout) {
    // Setup the timeout
    struct timeval time{};
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
    time.tv_sec = ms.count() / 1000;
    time.tv_usec = (ms.count() % 1000) * 1000;

    // Initialize the set that will contain the sockets that are ready
    socketsReadyForRead = allSockets;
    socketsReadyForWrite = allSockets;

    // Wait until one of the sockets is ready for reading, or timeout is reached
    int count = select(maxSocket + 1, &socketsReadyForRead, &socketsReadyForWrite, nullptr, ms.count() != 0 ? &time : nullptr);
    if (count < 0 && count != EINTR) {
        fprintf(stderr, "Failed to select: %d: %s", errno, strerror(errno));
    }
    return count > 0;
}

bool Selector::isReadyForRead(Socket &socket) const {
    HANDLE handle = socket.getHandle();
    if (handle != INVALID_SOCKET) {
        return FD_ISSET(handle, &socketsReadyForRead) != 0;
    }
    return false;
}

bool Selector::isReadyForWrite(Socket &socket) const {
    HANDLE handle = socket.getHandle();
    if (handle != INVALID_SOCKET) {
        return FD_ISSET(handle, &socketsReadyForWrite) != 0;
    }
    return false;
}
