//
// Created by l2pic on 17.05.2021.
//

#ifndef CHATCONTROLLER_HTTP_CLIENT_HTTPREQUESTFACTORY_H_
#define CHATCONTROLLER_HTTP_CLIENT_HTTPREQUESTFACTORY_H_

#include <string>
#include <string_view>
#include <map>

#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

#include "URI.h"
#include "HTTPRequest.h"

#define HTTP_VERSION 11
#define HTTP_CHUNK_SIZE 512

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>


struct HTTPRequestFactory {
    using Headers = std::map<std::string, std::string>;
    using Params = std::map<std::string, std::string>;

    static auto Request(const std::string& method = "GET",
                        const std::string& scheme = "http",
                        const std::string& host = "127.0.0.1",
                        unsigned short port = 80,
                        const std::string& path = "/",
                        const Headers &headers = Headers{},
                        const std::string& body = "",
                        bool keepAlive = false) {
        HTTPRequest request{};

        // configurations
        request->keep_alive(keepAlive);

        // form request uri
        request->method(http::string_to_verb(method));
        request->target(path);
        request.scheme(scheme);
        request.host(host);
        request.port(port);
        request->version(HTTP_VERSION);

        // headers
        request->set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        for (auto &[name, value] : headers) {
            request->set(name, value);
        }

        // body
        if (!body.empty()) {
            request->body() = body;
            request->content_length(body.size());
        }

        return request;
    }

    static auto Get(const URI& url) {
        return Request("GET", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery());
    }

    static auto Get(const URI& url, const Headers &headers) {
        return Request("GET", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers);
    }

    static auto Head(const URI& url) {
        return Request("HEAD", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery());
    }

    static auto Head(const URI& url, const Headers &headers) {
        return Request("HEAD", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers);
    }

    static auto Post(const URI& url) {
        return Request("POST", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery());
    }

    static auto Post(const URI& url, const char *body, size_t content_length, const char *content_type) {
        return Request("POST", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, std::string{body, content_length});
    }

    static auto Post(const URI& url, const Headers &headers, const char *body, size_t content_length) {
        return Request("POST", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       headers, std::string{body, content_length});
    }

    static auto Post(const URI& url, const std::string &body, const char *content_type) {
        return Request("POST", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, body);
    }

    static auto Post(const URI& url, const Headers &headers, const std::string &body) {
        return Request("POST", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers, body);
    }

    static auto Put(const URI& url) {
        return Request("PUT", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery());
    }

    static auto Put(const URI& url, const char *body, size_t content_length, const char *content_type) {
        return Request("PUT", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, std::string{body, content_length});
    }

    static auto Put(const URI& url, const Headers &headers, const char *body, size_t content_length) {
        return Request("POST", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       headers, std::string{body, content_length});
    }

    static auto Put(const URI& url, const std::string &body, const char *content_type) {
        return Request("PUT", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, body);
    }

    static auto Put(const URI& url, const Headers &headers, const std::string &body) {
        return Request("PUT", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers, body);
    }

    static auto Patch(const URI& url) {
        return Request("PATCH", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery());
    }

    static auto Patch(const URI& url, const char *body, size_t content_length, const char *content_type) {
        return Request("PATCH", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, std::string{body, content_length});
    }

    static auto Patch(const URI& url, const Headers &headers, const char *body, size_t content_length) {
        return Request("PATCH", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       headers, std::string{body, content_length});
    }

    static auto Patch(const URI& url, const std::string &body, const char *content_type) {
        return Request("PATCH", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, body);
    }

    static auto Patch(const URI& url, const Headers &headers, const std::string &body) {
        return Request("PATCH", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers, body);
    }

    static auto Delete(const URI& url) {
        return Request("DELETE", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery());
    }

    static auto Delete(const URI& url, const Headers &headers) {
        return Request("DELETE", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers);
    }

    static auto Delete(const URI& url, const char *body, size_t content_length, const char *content_type) {
        return Request("DELETE", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, std::string{body, content_length});
    }

    static auto Delete(const URI& url, const Headers &headers, const char *body, size_t content_length) {
        return Request("DELETE", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       headers, std::string{body, content_length});
    }

    static auto Delete(const URI& url, const std::string &body, const char *content_type) {
        return Request("DELETE", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(),
                       {{"Content-Type", content_type}}, body);
    }

    static auto Delete(const URI& url, const Headers &headers, const std::string &body) {
        return Request("DELETE", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers, body);
    }

    static auto Options(const URI& url) {
        return Request("OPTIONS", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery());
    }

    static auto Options(const URI& url, const Headers &headers) {
        return Request("OPTIONS", url.getScheme(), url.getHost(), url.getPort(), url.getPathAndQuery(), headers);
    }
};

#endif //CHATCONTROLLER_HTTP_CLIENT_HTTPREQUESTFACTORY_H_
