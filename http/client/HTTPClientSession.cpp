//
// Created by l2pic on 16.05.2021.
//

#include <strstream>

#include <boost/beast/version.hpp>

#include "../../common/Utils.h"
#include "../../common/Logger.h"

#include "HTTPClientSession.h"

HTTPClientSession::HTTPClientSession(net::any_io_executor ex, Logger *logger)
    : logger(logger),
      resolver(ex),
      stream(TCPStream{ex}) {
}

HTTPClientSession::HTTPClientSession(net::any_io_executor ex, ssl::context& ctx, Logger *logger)
  : logger(logger),
    resolver(ex),
    stream(SSLStream{ex, ctx}) {

}

HTTPClientSession::~HTTPClientSession() = default;

void HTTPClientSession::run(HTTPRequest &&httpRequest) {
    if (std::holds_alternative<SSLStream>(stream)) {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(std::get<SSLStream>(stream).native_handle(), httpRequest.host().data())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            logError(ec, "SSL SNI Hostname");
            return;
        }
    }
    // copy boost request
    request = std::move(httpRequest.native());

    std::ostringstream oss; oss << request;
    logger->logTrace("HTTPClientSession Outgoing request:\n{}", oss.str());

    // Look up the domain name
    resolver.async_resolve(httpRequest.host(), std::to_string(httpRequest.port()),
                           beast::bind_front_handler(&HTTPClientSession::onResolve, shared_from_this()));
}

void HTTPClientSession::onResolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec)
        return logError(ec, "resolve");


    // Set a timeout on the operation
    std::visit(overloaded {
        [](auto &s) { beast::get_lowest_layer(s).expires_after(std::chrono::seconds(30)); }
    }, stream);

    //beast::get_lowest_layer(stream_).async_connect(

    // Make the connection on the IP address we get from a lookup
    std::visit(overloaded {
        [&](auto &s) {
            beast::get_lowest_layer(s).async_connect(results, beast::bind_front_handler(&HTTPClientSession::onConnect,
                                                                                        shared_from_this()));
        }
    }, stream);
}

void HTTPClientSession::onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
    if (ec)
        return logError(ec, "connect");

    std::visit(overloaded {
        [this](TCPStream &s) {
            // Set a timeout on the operation
            s.expires_after(std::chrono::seconds(5));

            // Send the HTTP request to the remote host
            http::async_write(s, request, beast::bind_front_handler(&HTTPClientSession::onWrite, shared_from_this()));
        },
        [this](SSLStream &s) {
            // Perform the SSL handshake
            s.async_handshake(ssl::stream_base::client, beast::bind_front_handler(&HTTPClientSession::onHandshake,
                                                                                  shared_from_this()));
        }
    }, stream);
}

void HTTPClientSession::onHandshake(beast::error_code ec) {
    if (ec)
        return logError(ec, "handshake");

    assert(std::holds_alternative<SSLStream>(stream));

    auto& sslStream = std::get<SSLStream>(stream);
    // Set a timeout on the operation
    beast::get_lowest_layer(sslStream).expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    http::async_write(sslStream, request, beast::bind_front_handler(&HTTPClientSession::onWrite, shared_from_this()));
}

void HTTPClientSession::onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return logError(ec, "write");

    // Receive the HTTP response
    std::visit(overloaded {
        [this](auto &s) {
            http::async_read(s, buffer, response, beast::bind_front_handler(&HTTPClientSession::onRead, shared_from_this()));
        }
    }, stream);
}

void HTTPClientSession::onRead(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return logError(ec, "read");

    // Write the message to standard out
    std::ostringstream oss; oss << response;
    logger->logTrace("HTTPClientSession Incoming response:\n{}", oss.str());

    close();
}

void HTTPClientSession::close() {
    // Gracefully close the socket
    std::visit(overloaded {
        [this](TCPStream &s) {
            beast::error_code ec;
            s.socket().shutdown(tcp::socket::shutdown_both, ec);
            onShutdown(ec);
        },
        [this](SSLStream &s) {
            // Set the timeout.
            beast::get_lowest_layer(s).expires_after(std::chrono::seconds(30));
            // Perform the SSL shutdown
            s.async_shutdown(beast::bind_front_handler(&HTTPClientSession::onShutdown, shared_from_this()));
        }
    }, stream);
}

void HTTPClientSession::onShutdown(beast::error_code ec) {
    if (ec) {
        // Rationale for net::error::eof:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        if (ec == net::error::eof ||
            ec == beast::errc::not_connected) // not_connected happens sometimes so don't bother reporting it.
            return;
        return logError(ec, "Failed shutdown");
    }
    // If we get here then the connection is closed gracefully
}

void HTTPClientSession::logError(beast::error_code ec, std::string &&context) {
    if (ec == net::ssl::error::stream_truncated || ec == beast::error::timeout)
        return;
    logger->logError("HTTPClientSession {}: {}", context, ec.message());
}
