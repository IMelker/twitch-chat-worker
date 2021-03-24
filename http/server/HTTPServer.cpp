//
// Created by l2pic on 18.03.2021.
//

#include "absl/strings/str_split.h"

#include "HTTPServerUnit.h"
#include "HTTPServer.h"
#include "HTTPListener.h"
#include "HTTPResponseFactory.h"
#include "../../common/Logger.h"
#include "../../common/ThreadPool.h"

HTTPServer::HTTPServer(HTTPControlConfig config, std::shared_ptr<Logger> logger)
 : config(std::move(config)), logger(std::move(logger)), ioc(config.threads), executors(nullptr) {
    this->logger->logInfo("HTTPServer created with {} threads", this->config.threads);
}

HTTPServer::~HTTPServer() {
    ioc.stop();

    for (auto &runner: ioRunners) {
        if (runner.joinable())
            runner.join();
    }
    logger->logTrace("HTTPServer end of server");
    executors = nullptr;
};

bool HTTPServer::start(ThreadPool *pool) {
    this->executors = pool;

    boost::system::error_code ec;
    auto address = net::ip::make_address(config.host, ec);
    if (ec) {
        logger->logError("HTTPServer Failed to make ip address: {}", ec.message());
        return false;
    }

    // Create and launch a listening port
    listener = std::make_shared<HTTPListener>(ioc, tcp::endpoint{address, config.port}, logger, this);
    listener->startAcceptConnections();

    // Run the I/O service on the requested number of threads
    ioRunners.reserve(config.threads);
    for (int i = 0; i < config.threads; ++i) {
        ioRunners.emplace_back([&ioc = this->ioc] { ioc.run(); });
    }
    logger->logInfo("HTTPServer Started listening on {}:{} on {} threads", config.host, config.port, config.threads);
    return true;
}

void HTTPServer::handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) {
    std::vector<std::string_view> path = absl::StrSplit(req.target(), '/', absl::SkipWhitespace());
    if (path.size() < 2) {
        send(HTTPResponseFactory::BadRequest(req, "Invalid path"));
        return;
    }

    std::lock_guard lg(unitsMutex); // TODO not covering lambda, so fix data race in future
    auto it = units.find(path.front());
    if (it == units.end()) {
        send(HTTPResponseFactory::NotFound(req, req.target()));
        return;
    }

    if (!executors) {
        send(HTTPResponseFactory::ServerError(req, "There is no executor for request"));
        return;
    }

    executors->enqueue([req = std::move(req), send = std::move(send), &target = path[1], handler = it->second]() {
        std::string err;
        auto [status, body] = handler->processHttpRequest(target, req.body(), err);
        if (!err.empty()) {
            send(HTTPResponseFactory::ServerError(req, err));
            return;
        }

        http::response<http::string_body> res{static_cast<http::status>(status), req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = body;
        res.prepare_payload();

        send(std::move(res));
    });
}

void HTTPServer::addControlUnit(const std::string &path, HTTPServerUnit *unit) {
    logger->logInfo("HTTPServer Add request handle unit to HTTP server: {}", path);
    std::lock_guard lg(unitsMutex);
    units.emplace(path, unit);
}

void HTTPServer::deleteControlUnit(const std::string &path) {
    logger->logInfo("HTTPServer Remove request handle unit from HTTP server: {}", path);
    std::lock_guard lg(unitsMutex);
    units.erase(path);
}

void HTTPServer::clearUnits() {
    logger->logInfo("HTTPServer Clear all request handle units from HTTP server. Request handling stopped");
    std::lock_guard lg(unitsMutex);
    this->units.clear();
}
