//
// Created by l2pic on 22.03.2021.
//

#ifndef CHATSNIFFER_HTTP_SERVER_HTTPRESPONSEFACTORY_H_
#define CHATSNIFFER_HTTP_SERVER_HTTPRESPONSEFACTORY_H_

#include <string_view>
#include <boost/beast.hpp>

struct HTTPResponseFactory {
    template<class Request>
    static auto BadRequest(const Request& req, beast::string_view why) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    }

    template<class Request>
    static auto NotFound(const Request& req, beast::string_view target) {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    }

    template<class Request>
    static auto ServerError(const Request& req, beast::string_view what) {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    }
};

#endif //CHATSNIFFER_HTTP_SERVER_HTTPRESPONSEFACTORY_H_
