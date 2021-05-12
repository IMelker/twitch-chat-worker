//
// Created by l2pic on 06.05.2021.
//

#ifndef CHATCONTROLLER_COMMON_THREADNAME_H_
#define CHATCONTROLLER_COMMON_THREADNAME_H_

#include <cassert>
#include <thread>
#include <pthread.h>

inline int set_thread_name(std::thread& th, const std::string& name) {
    assert(name.size() <= 16);
    return pthread_setname_np(th.native_handle(), name.c_str());
}

inline int set_thread_name(const char *name) {
    assert(strlen(name) <= 16);
    return pthread_setname_np(pthread_self(), name);
}

#endif //CHATCONTROLLER_COMMON_THREADNAME_H_
