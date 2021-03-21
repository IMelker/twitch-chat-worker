//
// Created by imelker on 21.03.2021.
//

#include "HTTPSession.h"

static const int timeout = 30;

// This function produces an HTTP response for the given request.
// The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<class Send>
void handleRequest(http::request<http::string_body> &&req, Send &&send) {

    printf("%s\n", __PRETTY_FUNCTION__);

    // Returns a bad request response
    auto const bad_request =
        [&req](beast::string_view why) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        };

    // Returns a not found response
    auto const not_found =
        [&req](beast::string_view target) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        };

    // Returns a server error response
    auto const server_error =
        [&req](beast::string_view what) {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();
            return res;
        };

    // Make sure we can handle the method
    if (req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if (req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = "index.html";

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == beast::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if (ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head) {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/text");
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{std::piecewise_construct,
                                        std::make_tuple(std::move(body)),
                                        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/text");
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

HTTPSession::HTTPSession(tcp::socket &&socket, HTTPRequestHandler *handler)
    : tcpStream(std::move(socket)), handler(handler) {
    printf("%s\n", __PRETTY_FUNCTION__);
}

HTTPSession::~HTTPSession() = default;

void HTTPSession::run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    net::dispatch(tcpStream.get_executor(),
                  beast::bind_front_handler(&HTTPSession::read, shared_from_this()));
}

void HTTPSession::read() {
    printf("%s\n", __PRETTY_FUNCTION__);
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    request = {};

    // Set the timeout.
    tcpStream.expires_after(std::chrono::seconds(timeout));

    // Read a request
    http::async_read(tcpStream, inputBuffer, request,
                     beast::bind_front_handler(&HTTPSession::onRead, shared_from_this()));
}

void HTTPSession::onRead(beast::error_code ec, std::size_t bytes) {
    boost::ignore_unused(bytes);

    // This means they closed the connection
    if (ec == http::error::end_of_stream)
        return close();

    if (ec) {
        fprintf(stderr, "read: %s\n", ec.message().c_str());
        return;
    }

    // Move execution to other thread
    handler->handleRequest(std::move(request), SendLambda{shared_from_this()});

    // Send the response
    //handleRequest(std::move(request), SendLambda{shared_from_this()});
}

void HTTPSession::onWrite(bool close, beast::error_code ec, std::size_t bytes) {
    printf("%s\n", __PRETTY_FUNCTION__);
    boost::ignore_unused(bytes);

    if (ec) {
        fprintf(stderr, "write: %s\n", ec.message().c_str());
        return;
    }

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
    beast::error_code ec;
    tcpStream.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        fprintf(stderr, "close: %s\n", ec.message().c_str());
    }
}
