//
// Created by l2pic on 06.03.2021.
//

#include "SysSignal.h"

#include <csignal>
#include <sys/wait.h>

std::atomic<bool> SysSignal::terminated(false);

void SysSignal::setupSignalHandling() {
    std::signal(SIGTERM, SysSignal::signalHandler);
    std::signal(SIGSEGV, SysSignal::signalHandler);
    std::signal(SIGKILL, SysSignal::signalHandler);
    std::signal(SIGCHLD, SysSignal::signalHandler);
    std::signal(SIGUSR1, SysSignal::signalHandler);
    std::signal(SIGUSR2, SysSignal::signalHandler);
    std::signal(SIGINT, SysSignal::signalHandler);
}

void SysSignal::signalHandler(int signal_number) {
    switch (signal_number) {
        case SIGTERM:
        case SIGINT:
        case SIGSEGV:
            setServiceTerminated(true);
            break;
        case SIGKILL:
            setServiceTerminated(true);
            break;
        case SIGCHLD:
            while (waitpid(static_cast<pid_t>(-1), nullptr, WNOHANG) > 0);
            break;
        case SIGUSR1:
            setServiceTerminated(true);
            break;
        case SIGUSR2:
            setServiceTerminated(true);
            break;
        default:
            break;
    }
}

bool SysSignal::serviceTerminated() {
    return terminated.load(std::memory_order_relaxed);
}

void SysSignal::setServiceTerminated(bool state) {
    terminated.store(state, std::memory_order_relaxed);
}

