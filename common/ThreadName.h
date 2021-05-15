//
// Created by l2pic on 06.05.2021.
//

#ifndef CHATCONTROLLER_COMMON_THREADNAME_H_
#define CHATCONTROLLER_COMMON_THREADNAME_H_

#include <cassert>
#include <cstring>
#include <thread>
#include <pthread.h>

#if defined(__APPLE__) || defined(OS_SUNOS)
#elif defined(__FreeBSD__)
#include <pthread_np.h>
#else
#include <sys/prctl.h>
#endif


inline int set_thread_name(std::thread& th, const std::string& name) {
    assert(name.size() <= 16);
    return pthread_setname_np(th.native_handle(), name.c_str());
}

inline void set_thread_name(const char * name)
{
#ifndef NDEBUG
    assert(strlen(name) <= 16);
#endif

#if defined(OS_FREEBSD)
    pthread_set_name_np(pthread_self(), name);
    if ((false))
#elif defined(OS_DARWIN)
    if (0 != pthread_setname_np(name))
#elif defined(OS_SUNOS)
    if (0 != pthread_setname_np(pthread_self(), name))
#else
    if (0 != prctl(PR_SET_NAME, name, 0, 0, 0))
#endif
        fprintf(stderr, "Cannot set thread name with prctl(PR_SET_NAME, ...)");
}

inline std::string get_thread_name()
{
    std::string name(16, '\0');

#if defined(__APPLE__) || defined(OS_SUNOS)
    if (pthread_getname_np(pthread_self(), name.data(), name.size()))
        fprintf(stderr, "Cannot get thread name with pthread_getname_np()");
#elif defined(__FreeBSD__)
    // TODO: make test. freebsd will have this function soon https://freshbsd.org/commit/freebsd/r337983
//    if (pthread_get_name_np(pthread_self(), name.data(), name.size()))
//        fprintf(stderr, "Cannot get thread name with pthread_get_name_np()");
#else
    if (0 != prctl(PR_GET_NAME, name.data(), 0, 0, 0))
        fprintf(stderr, "Cannot get thread name with prctl(PR_GET_NAME)");
#endif

    name.resize(std::strlen(name.data()));
    return name;
}

#endif //CHATCONTROLLER_COMMON_THREADNAME_H_
