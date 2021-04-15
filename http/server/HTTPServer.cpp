//
// Created by imelker on 18.03.2021.
//

#include <exception>

#include "../../common/Logger.h"

#include "HTTPServerUnit.h"
#include "HTTPServer.h"
#include "HTTPListener.h"
#include "HTTPServerCertificate.h"

HTTPServer::HTTPServer(HTTPControlConfig config, HTTPRequestHandler *handler, std::shared_ptr<Logger> logger)
    : config(std::move(config)),
      logger(std::move(logger)),
      ioc(config.threads),
      ctx(boost::asio::ssl::context::tlsv12),
      handler(handler) {
    if (this->config.secure) {
        if (this->config.ssl.verify)
            ctx.set_verify_mode(boost::asio::ssl::verify_fail_if_no_peer_cert);
        else
            ctx.set_verify_mode(boost::asio::ssl::verify_none);

        if (std::string err; !loadServerCertificate(ctx, err, &this->config.ssl.cert,
                                                              &this->config.ssl.key,
                                                              &this->config.ssl.dh)) {
            this->logger->logCritical("HTTPServer Failed to load SSL certificates: {}", err);
            throw err;
        }
        this->logger->logInfo("HTTPServer SSL certificates loaded");
    }
    this->logger->logInfo("HTTPServer created with {} threads", this->config.threads);
}

HTTPServer::~HTTPServer() {
    stop();

    for (auto &runner: ioRunners) {
        if (runner.joinable())
            runner.join();
    }
    logger->logTrace("HTTPServer end of server");
};

bool HTTPServer::start() {
    boost::system::error_code ec;
    auto address = net::ip::make_address(config.host, ec);
    if (ec) {
        logger->logError("HTTPServer Failed to make ip address: {}", ec.message());
        return false;
    }

    // Create and launch a listening port
    if (config.secure) {
        listener = std::make_shared<HTTPListener>(ioc, &ctx, tcp::endpoint{address, config.port}, logger, this);
    } else {
        listener = std::make_shared<HTTPListener>(ioc, tcp::endpoint{address, config.port}, logger, this);
    }
    listener->startAcceptConnections();

    // Run the I/O service on the requested number of threads
    ioRunners.reserve(config.threads);
    for (unsigned int i = 0; i < config.threads; ++i) {
        ioRunners.emplace_back([&ioc = this->ioc] { ioc.run(); });
    }
    logger->logInfo("HTTPServer Started listening on {}:{} on {} threads", config.host, config.port, config.threads);
    return true;
}

void HTTPServer::stop() {
    ioc.stop();
}

void HTTPServer::handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) {
    handler->handleRequest(std::move(req), std::move(send));
}
