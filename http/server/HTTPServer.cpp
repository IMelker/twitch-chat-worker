//
// Created by imelker on 18.03.2021.
//

#include <exception>

#include "../../common/Logger.h"
#include "../../common/ThreadName.h"
#include "../details/Details.h"
#include "../details/RootCertificates.h"
#include "HTTPServer.h"
#include "HTTPListener.h"
#include "HTTPResponseFactory.h"

HTTPServer::HTTPServer(HTTPServerConfig config, HTTPRequestHandler *handler, std::shared_ptr<Logger> logger)
    : config(std::move(config)),
      logger(std::move(logger)),
      ioc(this->config.threads),
      ctx(boost::asio::ssl::context::tlsv12),
      handler(handler) {
    if (!this->config.pass.empty()) {
        basicAuth = base64::encode64(this->config.user + ":" + this->config.pass);
    }

    if (this->config.secure) {
        if (this->config.ssl.verify)
            ctx.set_verify_mode(boost::asio::ssl::verify_fail_if_no_peer_cert);
        else
            ctx.set_verify_mode(boost::asio::ssl::verify_none);

        if (std::string err; !loadRootCertificates(ctx, err, &this->config.ssl.cert,
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
        auto &thread = ioRunners.emplace_back([&ioc = this->ioc] { ioc.run(); });
        set_thread_name(thread, "http_server_" + std::to_string(i));
    }
    logger->logInfo("HTTPServer Started listening on {}:{} on {} threads", config.host, config.port, config.threads);
    return true;
}

void HTTPServer::stop() {
    ioc.stop();
}

void HTTPServer::handleRequest(http::request<http::string_body> &&req, HTTPServerSession::SendLambda &&send) {
    if (!basicAuth.empty()) {
        auto it = req.find("Authorization");
        if (it == req.end()) {
            return send(HTTPResponseFactory::Unauthorized(req));
        }
        else {
            auto pos = it->value().find(' ');

            auto type =  it->value().substr(0, pos);
            if (type != "Basic")
                return send(HTTPResponseFactory::NotAcceptable(req));

            auto encoded = it->value().substr(pos + 1);
            if (encoded != basicAuth)
                return send(HTTPResponseFactory::Forbidden(req));
            // auth succeed
        }
    }

    handler->handleRequest(std::move(req), std::move(send));
}
