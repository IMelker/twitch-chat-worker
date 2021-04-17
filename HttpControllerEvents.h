//
// Created by l2pic on 15.04.2021.
//

#ifndef CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_
#define CHATCONTROLLER__HTTPCONTROLLEREVENTS_H_

#include "http/server/HTTPSession.h"
#include <so_5/message.hpp>

namespace hreq {

struct base {
    mutable http::request<http::string_body> req;
    mutable HTTPSession::SendLambda send;
};

struct resp : public base {
    int status = 200;
    mutable std::string body;
};

// handled by Controller
namespace app {
struct shutdown : public base {};
struct version : public base {};
}

// handled by BotsEnvironment
namespace bot {
struct add : public base {};
struct remove : public base {};
struct reload : public base {};
struct reloadall : public base {};
}

// handled by Storage
namespace storage {
struct stats : public base {};
}

// handled by HttpController
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
