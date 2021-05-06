//
// Created by l2pic on 06.05.2021.
//

#ifndef CHATCONTROLLER_COMMON_THREADNAME_H_
#define CHATCONTROLLER_COMMON_THREADNAME_H_

#include <thread>
#include <pthread.h>

inline int set_thread_name(std::thread& th, const std::string& name) {
    return pthread_setname_np(th.native_handle(), name.c_str());
}

#endif //CHATCONTROLLER_COMMON_THREADNAME_H_
