//
// Created by l2pic on 13.04.2021.
//

#include <string_view>
#include <absl/strings/str_split.h>

#include "server/HTTPResponseFactory.h"
#include "../common/ThreadPool.h"

#include "HTTPServerAgent.h"

HTTPServerAgent::HTTPServerAgent(const context_t &ctx,
                                 HTTPControlConfig config,
                                 std::shared_ptr<Logger> logger)
  : so_5::agent_t(ctx), HTTPServer(std::move(config), std::move(logger)) {

}

HTTPServerAgent::~HTTPServerAgent() {

}


void HTTPServerAgent::handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) {
    std::vector<std::string_view> path = absl::StrSplit(std::string_view(req.target().data(), req.target().size()),
                                                        '/', absl::SkipWhitespace());
    if (path.size() < 2) {
        send(HTTPResponseFactory::BadRequest(req, "Invalid path"));
        return;
    }

    std::shared_lock sl(unitsMutex); // TODO not covering lambda, so fix data race in future
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
        int status = 200;
        std::string err;
        std::string body;
        try {
            std::tie(status, body) = handler->processHttpRequest(target, req.body(), err);
        } catch (const std::exception& e) {
            send(HTTPResponseFactory::ServerError(req, e.what()));
            return;
        } catch (...) {
            send(HTTPResponseFactory::ServerError(req, err));
        }

        if (!err.empty()) {
            send(HTTPResponseFactory::ServerError(req, err));
            return;
        }

        send(HTTPResponseFactory::CreateResponse(req, static_cast<http::status>(status), std::move(body)));
    });
}
