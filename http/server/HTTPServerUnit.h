//
// Created by imelker on 18.03.2021.
//

#ifndef CHATSNIFFER_HTTP_HTTPSERVERUNIT_H_
#define CHATSNIFFER_HTTP_HTTPSERVERUNIT_H_

#include <string>
#include <tuple>

#define EMPTY_HTTP_RESPONSE std::tuple<int, std::string>{0, ""}

struct HTTPServerUnit {
    virtual std::tuple<int, std::string> processHttpRequest(std::string_view path,
                                                            const std::string& body,
                                                            std::string &error) = 0;
};

#endif //CHATSNIFFER_HTTP_HTTPSERVERUNIT_H_
