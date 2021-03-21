//
// Created by l2pic on 21.03.2021.
//

#include "HTTPListener.h"

#include "HTTPSession.h"

HTTPListener::HTTPListener(net::io_context &ioc, tcp::endpoint endpoint, HTTPRequestHandler *handler)
    : ioc(ioc), endpoint(endpoint), acceptor(net::make_strand(ioc)), handler(handler) {
    beast::error_code ec;

    // Open the acceptor
    acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        fprintf(stderr, "open: %s\n", ec.message().c_str());
        return;
    }

    // Allow address reuse
    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        fprintf(stderr, "set_option: %s\n", ec.message().c_str());
        return;
    }

    // Bind to the server address
    acceptor.bind(endpoint, ec);
    if (ec) {
        fprintf(stderr, "bind: %s\n", ec.message().c_str());
        return;
    }

    // Start listening for connections
    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        fprintf(stderr, "listen: %s\n", ec.message().c_str());
        return;
    }
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
    printf("%s\n", __PRETTY_FUNCTION__);

    if (!ec) { // Create the session and run it
        std::make_shared<HTTPSession>(std::move(socket), handler)->run();
    } else {
        fprintf(stderr, "accept: %s\n", ec.message().c_str());
    }

    accept(); // Accept another connection
}
