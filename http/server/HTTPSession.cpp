//
// Created by imelker on 21.03.2021.
//

#include "HTTPSession.h"
#include "HTTPRequestHandler.h"

static const int timeout = 30;

HTTPSession::HTTPSession(tcp::socket &&socket, HTTPRequestHandler *handler, std::shared_ptr<Logger> logger)
    : logger(std::move(logger)), stream(TCPStream{std::move(socket)}), handler(handler) {
}

HTTPSession::HTTPSession(tcp::socket &&socket, ssl::context *ctx, HTTPRequestHandler *handler, std::shared_ptr<Logger> logger)
    : logger(std::move(logger)), stream(SSLStream{std::move(socket), *ctx}), handler(handler) {
}

HTTPSession::~HTTPSession() = default;

void HTTPSession::run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    std::visit(overloaded {
        [this](auto &s) { net::dispatch(s.get_executor(), beast::bind_front_handler(&HTTPSession::start, shared_from_this())); },
    }, stream);
}

void HTTPSession::start() {
    // Set the timeout.
    std::visit(overloaded {
        [](auto &s) { beast::get_lowest_layer(s).expires_after(std::chrono::seconds(timeout)); }
    }, stream);

    std::visit(overloaded {
        [this](TCPStream &s) {
            // Set the timeout.
            s.expires_after(std::chrono::seconds(timeout));

            // Init read stream
            read();
        },
        [this](SSLStream &s) {
            // Set the timeout.
            beast::get_lowest_layer(s).expires_after(std::chrono::seconds(timeout));

            // Perform the SSL handshake
            s.async_handshake(ssl::stream_base::server, beast::bind_front_handler(&HTTPSession::handshake, shared_from_this()));
        }
    }, stream);
}

void HTTPSession::handshake(beast::error_code ec) {
    if (ec) { return logError(ec, "Failed handshake"); }

    read();
}

void HTTPSession::read() {
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    request = {};

    // Set the timeout.
    std::visit(overloaded {
        [](auto &s) { beast::get_lowest_layer(s).expires_after(std::chrono::seconds(timeout)); }
    }, stream);

    // Read a request
    std::visit(overloaded {
        [this](auto &s) { http::async_read(s, buffer, request, beast::bind_front_handler(&HTTPSession::onRead, shared_from_this())); }
    }, stream);
}

void HTTPSession::onRead(beast::error_code ec, std::size_t) {
    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return close();

    if (ec) { return logError(ec, "Failed read"); }

    std::ostringstream oss; oss << request;
    logger->logTrace("HTTPSession Incoming request:\n{}", oss.str());

    // Move execution to other thread
    handler->handleRequest(std::move(request), SendLambda{shared_from_this(), logger});
}

void HTTPSession::onWrite(bool close, beast::error_code ec, std::size_t) {
    if (ec) { return logError(ec, "Failed write"); }

    if (close) {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return this->close();
    }

    // We're done with the response so delete it
    response.reset();

    // Read another request
    read();
}

void HTTPSession::close() {
    std::visit(overloaded {
        [this](TCPStream &s) {
            beast::error_code ec;
            s.socket().shutdown(tcp::socket::shutdown_send, ec);
            shutdown(ec);
        },
        [this](SSLStream &s) {
            // Set the timeout.
            beast::get_lowest_layer(s).expires_after(std::chrono::seconds(timeout));

            // Perform the SSL shutdown
            s.async_shutdown(beast::bind_front_handler(&HTTPSession::shutdown, shared_from_this()));
        }
    }, stream);
}

void HTTPSession::shutdown(beast::error_code ec) {
    if (ec) { return logError(ec, "Failed shutdown"); }
}

void HTTPSession::logError(beast::error_code ec, std::string &&context) {
    if (ec == net::ssl::error::stream_truncated || ec == beast::error::timeout)
        return;

    logger->logError("HTTPSession {}: {}", context, ec.message());
}

