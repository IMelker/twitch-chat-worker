//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_
#define CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_

#include "http/server/HTTPSession.h"
#include <so_5/message.hpp>

namespace hreq {

struct base {
    http::request<http::string_body> req;
    HTTPSession::SendLambda send;
};

struct resp : public base {
    int status = 200;
    std::string body;
};

namespace app {
struct shutdown : public base {};
struct version : public base {};
}

namespace bot {
struct add : public base {};
struct remove : public base {};
struct reload : public base {};
}

namespace storage {
struct stats : public base {};
}

namespace db {
struct stats : public base {};
}

namespace irc::channel {
struct stats : public base {};
struct reload : public base {};
struct join : public base {};
struct leave : public base {};
struct message : public base {};
struct custom : public base {};
}

namespace irc::account {
struct stats : public base {};
struct reload : public base {};
}

}

#endif //CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_
