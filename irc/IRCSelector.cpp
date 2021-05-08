//
// Created by l2pic on 25.04.2021.
//

#include <cstring>
#include <string>
#include <chrono>

#include <libircclient.h>

#include "Logger.h"
#include "ThreadName.h"

#include "IRCSession.h"
#include "IRCSelector.h"

#define SELECT_DELAY 1000

IRCSelector::IRCSelector(size_t id, Logger *logger) : id(id), logger(logger) {
    thread = std::thread(&IRCSelector::run, this);
    set_thread_name(thread, "irc_selector_" + std::to_string(id));
    loggerTag = fmt::format("IRCSelector[{}/{}]", fmt::ptr(this), id);
    logger->logTrace("{} IRC selector init", loggerTag);
}

IRCSelector::~IRCSelector() {
    if (thread.joinable())
        thread.join();
    logger->logTrace("{} IRC selector destruction", loggerTag);
}

void IRCSelector::addSession(const std::shared_ptr<IRCSession> &session) {
    logger->logTrace("{} IRC selector add new session({})", loggerTag, fmt::ptr(session.get()));

    std::lock_guard lg(mutex);
    sessions.push_back(session);
    needSync = true;
}

void IRCSelector::removeSession(const std::shared_ptr<IRCSession> &session) {
    logger->logTrace("{} IRC selector remove session({})", loggerTag, fmt::ptr(session.get()));

    std::lock_guard lg(mutex);
    sessions.erase(std::find(sessions.begin(), sessions.end(), session));
    needSync = true;
}

void IRCSelector::run() {
    // Setup the timeout
    int maxfd = 0;
    fd_set in_set, out_set;
    struct timeval tv{};

    while (!SysSignal::serviceTerminated()) {
        maxfd = 0;

        tv.tv_sec = SELECT_DELAY / 1000;
        tv.tv_usec = (SELECT_DELAY % 1000) * 1000;

        // Init sets
        FD_ZERO(&in_set);
        FD_ZERO(&out_set);

        if (std::lock_guard lg(mutex); needSync) {
            threadSafeCopy = sessions;
            needSync = false;
        }

        for (auto &session : threadSafeCopy) {
            if (!session->connected())
                continue;

            if (irc_add_select_descriptors(session->session, &in_set, &out_set, &maxfd)) {
                logger->logError("{} Failed to add session to select list: {}",
                                 loggerTag, irc_strerror(irc_errno(session->session)));
            }
        }

        if (maxfd == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        int count = select(maxfd + 1, &in_set, &out_set, nullptr, &tv);
        if (count < 0 && errno != EINTR) {
            logger->logError("{} Failed to select: {} {}", loggerTag, errno, strerror(errno));
        }

        for (auto &session : threadSafeCopy) {
            if (irc_process_select_descriptors(session->session, &in_set, &out_set)) {
                logger->logError("{} Failed to process select list: {}",
                                 loggerTag, irc_strerror(irc_errno(session->session)));
            }
        }
    }
}
