//
// Created by imelker on 21.03.2021.
//

#include "HTTPSession.h"
#include "HTTPListener.h"

HTTPListener::HTTPListener(net::io_context &ioc, tcp::endpoint endpoint,
                           std::shared_ptr<Logger> logger, HTTPRequestHandler *handler)
    : logger(std::move(logger)), ioc(ioc), ctx(nullptr), endpoint(std::move(endpoint)), acceptor(net::make_strand(ioc)), handler(handler) {
    beast::error_code ec;

    // Open the acceptor
    acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        this->logger->logError("HTTPListener Failed open: {}", ec.message());
        return;
    }

    // Allow address reuse
    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        this->logger->logError("HTTPListener Failed set option: {}", ec.message());
        return;
    }

    // Bind to the server address
    acceptor.bind(endpoint, ec);
    if (ec) {
        this->logger->logError("HTTPListener Failed to bind: {}", ec.message());
        return;
    }

    // Start listening for connections
    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        this->logger->logError("HTTPListener Failed to listen: {}", ec.message());
        return;
    }
}

HTTPListener::HTTPListener(net::io_context &ioc, ssl::context *ctx, tcp::endpoint endpoint,
                           std::shared_ptr<Logger> logger, HTTPRequestHandler *handler)
    : HTTPListener(ioc, std::move(endpoint), std::move(logger), handler) {
    this->ctx = ctx;
}


HTTPListener::~HTTPListener() = default;

void HTTPListener::startAcceptConnections() {
    accept();
}

void HTTPListener::accept() {
    acceptor.async_accept(net::make_strand(ioc), // The new connection gets its own strand
                          beast::bind_front_handler(&HTTPListener::onAccept, shared_from_this()));
}

void HTTPListener::onAccept(beast::error_code ec, tcp::socket socket) {
    if (!ec) { // Create the session and run it
        if (ctx)
            std::make_shared<HTTPSession>(std::move(socket), ctx, handler, logger)->run();
        else
            std::make_shared<HTTPSession>(std::move(socket), handler, logger)->run();
    } else {
        logger->logError("HTTPListener {}", ec.message());
    }

    accept(); // Accept another connection
}
