//
// Created by l2pic on 17.05.2021.
//

#ifndef CHATCONTROLLER_HTTP_CLIENT_HTTPREQUEST_H_
#define CHATCONTROLLER_HTTP_CLIENT_HTTPREQUEST_H_

#include <string_view>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>

using HTTPRequestString = http::request<http::string_body>;

class HTTPRequest
{
  public:
    HTTPRequest() = default;
    ~HTTPRequest() = default;

    HTTPRequestString * operator->() {
        return &request_;
    }

    HTTPRequestString& native() {
        return request_;
    }

    void scheme(std::string_view scheme) {
        scheme_ = scheme;
    }

    const std::string& scheme() const {
        return scheme_;
    }

    void host(std::string_view host) {
        request_.set(http::field::host, host.data());
        host_ = host;
    }

    const std::string& host() const {
        return host_;
    }

    void port(unsigned short port) {
        port_ = port;
    }

    unsigned short port() const {
        return port_;
    }

  private:
    std::string scheme_;
    std::string host_;
    unsigned short port_;
    HTTPRequestString request_;
};

#endif //CHATCONTROLLER_HTTP_CLIENT_HTTPREQUEST_H_
