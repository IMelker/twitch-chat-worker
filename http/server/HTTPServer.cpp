//
// Created by l2pic on 18.03.2021.
//

#include "HTTPServerUnit.h"
#include "HTTPServer.h"
#include "HTTPListener.h"
#include "HTTPResponseFactory.h"
#include "../../common/ThreadPool.h"

HTTPServer::HTTPServer(HTTPControlConfig config, std::shared_ptr<Logger> logger)
 : config(std::move(config)), logger(std::move(logger)), ioc(config.threads), executors(nullptr) {
}

HTTPServer::~HTTPServer() {
    for (auto &runner: ioRunners) {
        if (runner.joinable())
            runner.join();
    }
};

bool HTTPServer::start(ThreadPool *pool) {
    this->executors = pool;

    boost::system::error_code ec;
    auto address = net::ip::make_address(this->config.host, ec);
    if (ec) {
        fprintf(stderr, "make_address: %s\n", ec.message().c_str());
        return false;
    }

    // Create and launch a listening port
    listener = std::make_shared<HTTPListener>(ioc, tcp::endpoint{address, this->config.port}, this);
    listener->startAcceptConnections();

    // Run the I/O service on the requested number of threads
    ioRunners.reserve(config.threads);
    for (int i = 0; i < config.threads; ++i) {
        ioRunners.emplace_back([&ioc = this->ioc] { ioc.run(); });
    }
}

inline std::string_view toSV(const beast::string_view& view) {
    return std::string_view{view.data(), view.size()};
}

void HTTPServer::handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) {
    std::lock_guard lg(unitsMutex); // TODO not covering lambda, so fix data race in future
    auto it = units.find(req.target());
    if (it == units.end()) {
        send(HTTPResponseFactory::NotFound(req, req.target()));
        return;
    }

    executors->enqueue([req = std::move(req), send = std::move(send), handler = it->second]() {
        std::string err;
        auto [status, body] = handler->processHttpRequest(toSV(req.target()), req.body(), err);
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
    std::lock_guard lg(unitsMutex);
    units.emplace(path, unit);
}

void HTTPServer::deleteControlUnit(const std::string &path) {
    std::lock_guard lg(unitsMutex);
    units.erase(path);
}

std::tuple<int, std::string> HTTPServer::processHttpRequest(std::string_view path, const std::string &body, std::string &error) {
    return {200, body};
}

void HTTPServer::stop() {
    std::lock_guard lg(unitsMutex);
    this->units.clear();
}
