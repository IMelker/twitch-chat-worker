//
// Created by l2pic on 25.04.2021.
//

#include <cstring>
#include <chrono>

#include <libircclient.h>

#include "IRCSession.h"
#include "IRCSelector.h"

#define SELECT_DELAY 1000

IRCSelector::IRCSelector(size_t id) : id(id) {
    thread = std::thread(&IRCSelector::run, this);
}

IRCSelector::~IRCSelector() {
    if (thread.joinable())
        thread.join();
}

void IRCSelector::addSession(const std::shared_ptr<IRCSession> &session) {
    std::lock_guard lg(mutex);
    sessions.push_back(session);
    needSync = true;
}

void IRCSelector::removeSession(const std::shared_ptr<IRCSession> &session) {
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
                //fprintf(stderr, "IRCWorker Failed to add session to select list: %s\n",
                //        irc_strerror(irc_errno(session->session)));
            }
        }

        if (maxfd == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        int count = select(maxfd + 1, &in_set, &out_set, nullptr, &tv);
        if (count < 0 && count != EINTR) {
            fprintf(stderr, "Failed to select: %d: %s\n", errno, strerror(errno));
        }

        for (auto &session : threadSafeCopy) {
            if (irc_process_select_descriptors(session->session, &in_set, &out_set)) {
                //fprintf(stderr, "IRCWorker Failed to process select list: %s\n",
                //        irc_strerror(irc_errno(session->session)));
            }
        }
    }
}
