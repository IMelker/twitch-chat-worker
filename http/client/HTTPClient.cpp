//
// Created by l2pic on 15.04.2021.
//

#include <boost/asio/strand.hpp>

#include "../../common/Logger.h"
#include "../../common/ThreadName.h"

#include "../details/RootCertificates.h"
#include "HTTPClientSession.h"

#include "HTTPClient.h"

HTTPClient::HTTPClient(HTTPClientConfig config, std::shared_ptr<Logger> logger)
  : config(std::move(config)),
    logger(std::move(logger)),
    ctx(ssl::context::tlsv12_client),
    work(ioService) {
    if (this->config.secure) {
        if (this->config.ssl.verify)
            ctx.set_verify_mode(boost::asio::ssl::verify_fail_if_no_peer_cert);
        else
            ctx.set_verify_mode(boost::asio::ssl::verify_none);

        if (std::string err; !loadRootCertificates(ctx, err, &this->config.ssl.cert,
                                                   &this->config.ssl.key,
                                                   &this->config.ssl.dh)) {
            this->logger->logCritical("HTTPClient Failed to load SSL certificates: {}", err);
            throw err;
        }
        this->logger->logInfo("HTTPClient SSL certificates loaded");
    }
    this->logger->logInfo("HTTPClient created with {} threads", this->config.threads);
}

HTTPClient::~HTTPClient() {
    stop();
    threadPool.join_all();
    logger->logTrace("HTTPClient end of server");
}

bool HTTPClient::start() {
    // Run the I/O service on the requested number of threads
    for (unsigned int i = 0; i < config.threads; ++i) {
        auto * thread = threadPool.create_thread([ioService = &ioService] { ioService->run(); });
        set_thread_name(thread->native_handle(), "http_client_" + std::to_string(i));
    }
    logger->logInfo("HTTPClient Started on {} threads", config.threads);
    return true;
}

void HTTPClient::stop() {
    ioService.stop();
}

void HTTPClient::exec(HTTPRequest &&request) {
    ioService.post([request = std::move(request), this] () mutable {
        bool https = request.scheme() == "https" || request.port() == 443;

        net::io_context ioc;
        std::shared_ptr<HTTPClientSession> session;
        if (https) {
            session = std::make_shared<HTTPClientSession>(net::make_strand(ioc), ctx, logger.get());
        } else {
            session = std::make_shared<HTTPClientSession>(net::make_strand(ioc), logger.get());
        }
        session->run(std::move(request));
        ioc.run();
    });
}

