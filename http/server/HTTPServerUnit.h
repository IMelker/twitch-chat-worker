//
// Created by l2pic on 18.03.2021.
//

#ifndef CHATSNIFFER_HTTP_HTTPSERVERUNIT_H_
#define CHATSNIFFER_HTTP_HTTPSERVERUNIT_H_

#include <string>
#include <tuple>

struct HTTPServerUnit {
    virtual std::tuple<int, std::string> processHttpRequest(std::string_view path,
                                                            const std::string& body,
                                                            std::string &error) = 0;
};

#endif //CHATSNIFFER_HTTP_HTTPSERVERUNIT_H_
