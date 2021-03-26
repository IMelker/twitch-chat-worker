//
// Created by l2pic on 22.03.2021.
//

#ifndef CHATSNIFFER_HTTP_SERVER_HTTPRESPONSEFACTORY_H_
#define CHATSNIFFER_HTTP_SERVER_HTTPRESPONSEFACTORY_H_

#include <string_view>
#include <boost/beast.hpp>
#include "nlohmann/json.hpp"

using json = nlohmann::json;


struct HTTPResponseFactory {
    template<class Request>
    static auto CreateResponse(const Request& req, http::status status, std::string&& body) {
        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = std::move(body);
        res.prepare_payload();
        return res;
    }

    template<class Request>
    static auto OK(const Request& req, beast::string_view body) {
        return CreateResponse(req, http::status::ok, body);
    }

    template<class Request>
    static auto BadRequest(const Request& req, beast::string_view why) {
        return CreateResponse(req, http::status::bad_request, json{{"error", why}}.dump());
    }

    template<class Request>
    static auto Unauthorized(const Request& req) {
        return CreateResponse(req, http::status::unauthorized, json{{"error", "Unauthorized"}}.dump());
    }

    template<class Request>
    static auto PaymentRequired(const Request& req) {
        return CreateResponse(req, http::status::payment_required, json{{"error", "PaymentRequired"}}.dump());
    }

    template<class Request>
    static auto Forbidden(const Request& req) {
        return CreateResponse(req, http::status::forbidden, json{{"error", "Forbidden"}}.dump());
    }

    template<class Request>
    static auto NotFound(const Request& req, beast::string_view target) {
        return CreateResponse(req, http::status::not_found, json{{"error", fmt::format("Failed to find resource '{}'", target)}}.dump());
    }

    template<class Request>
    static auto MethodNotAllowed(const Request& req) {
        return CreateResponse(req, http::status::method_not_allowed, json{{"error", "Method not allowed"}}.dump());
    }

    template<class Request>
    static auto NotAcceptable(const Request& req) {
        return CreateResponse(req, http::status::not_acceptable, json{{"error", "Not acceptable here"}}.dump());
    }

    //proxy_authentication_required       = 407,

    template<class Request>
    static auto RequestTimeout(const Request& req) {
        return CreateResponse(req, http::status::request_timeout, json{{"error", "Request timeout"}}.dump());
    }

    //conflict                            = 409,
    //gone                                = 410,
    //length_required                     = 411,
    //precondition_failed                 = 412,
    //payload_too_large                   = 413,
    //uri_too_long                        = 414,
    //unsupported_media_type              = 415,
    //range_not_satisfiable               = 416,
    //expectation_failed                  = 417,
    //misdirected_request                 = 421,
    //unprocessable_entity                = 422,
    //locked                              = 423,
    //failed_dependency                   = 424,
    //upgrade_required                    = 426,
    //precondition_required               = 428,
    //too_many_requests                   = 429,
    //request_header_fields_too_large     = 431,
    //connection_closed_without_response  = 444,
    //unavailable_for_legal_reasons       = 451,
    //client_closed_request               = 499,

    template<class Request>
    static auto ServerError(const Request& req, beast::string_view what) {
        return CreateResponse(req, http::status::internal_server_error, json{{"error", what}}.dump());
    }

    template<class Request>
    static auto NotImplemented(const Request& req, beast::string_view what) {
        return CreateResponse(req, http::status::not_implemented, json{{"error", fmt::format("Not implemented: {}", what)}}.dump());
    }

    template<class Request>
    static auto BadGateway(const Request& req) {
        return CreateResponse(req, http::status::bad_gateway, json{{"error", "Bad gateway"}}.dump());
    }

    template<class Request>
    static auto ServiceUnavailable(const Request& req) {
        return CreateResponse(req, http::status::bad_gateway, json{{"error", "Service unavailable"}}.dump());
    }

    //gateway_timeout                     = 504,
    //http_version_not_supported          = 505,
    //variant_also_negotiates             = 506,
    //insufficient_storage                = 507,
    //loop_detected                       = 508,
    //not_extended                        = 510,
    //network_authentication_required     = 511,
    //network_connect_timeout_error       = 599
};

#endif //CHATSNIFFER_HTTP_SERVER_HTTPRESPONSEFACTORY_H_
