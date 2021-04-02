//
// Created by imelker on 13.03.2021.
//

#ifndef CHATSNIFFER_COMMON_SINGLETON_H_
#define CHATSNIFFER_COMMON_SINGLETON_H_

template <typename T>
T* Singleton() {
    static T instance;
    return &instance;
}

#endif //CHATSNIFFER_COMMON_SINGLETON_H_
