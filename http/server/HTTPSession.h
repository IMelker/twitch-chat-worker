//
// Created by l2pic on 21.03.2021.
//

#ifndef CHATSNIFFER_HTTP_HTTPSESSION_H_
#define CHATSNIFFER_HTTP_HTTPSESSION_H_

#include <memory>
#include <variant>

#include "../../common/Logger.h"

#include <boost/asio.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>

// helper type for stream visitor
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


struct HTTPRequestHandler;
class HTTPSession : public std::enable_shared_from_this<HTTPSession>
{
    using TCPStream = beast::tcp_stream;
    using SSLStream = beast::ssl_stream<beast::tcp_stream>;
  public:
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct SendLambda
    {
        explicit SendLambda(std::shared_ptr<HTTPSession> holder, std::shared_ptr<Logger> logger)
          : holder(std::move(holder)), logger(std::move(logger)) {}

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            holder->response = sp;

            std::ostringstream oss; oss << *sp;
            logger->logTrace("HTTPServer Outgoing response:\n{}", oss.str());

            // Write the response
            std::visit(overloaded {
                [this, r = std::move(sp)](auto &s) {
                    http::async_write(s, *r, beast::bind_front_handler(&HTTPSession::onWrite, holder, r->need_eof()));
                }
            }, holder->stream);
        }

        std::shared_ptr<HTTPSession> holder;
        std::shared_ptr<Logger> logger;
    };
  public:
    // Take ownership of the stream
    explicit HTTPSession(tcp::socket &&socket, HTTPRequestHandler *handler, std::shared_ptr<Logger> logger);
    explicit HTTPSession(tcp::socket &&socket, ssl::context* ctx, HTTPRequestHandler *handler, std::shared_ptr<Logger> logger);
    ~HTTPSession();

    // Start the asynchronous operation
    void run();
    void start();

    // SSL handshake
    void handshake(beast::error_code ec);

    // Read request to object
    void read();
    void onRead(beast::error_code ec, std::size_t bytes);

    void onWrite(bool close, beast::error_code ec, std::size_t bytes);

    void close();
    void shutdown(beast::error_code ec);
  private:
    void logError(beast::error_code ec, std::string &&context);

    std::shared_ptr<Logger> logger;

    std::variant<TCPStream, SSLStream> stream;
    beast::flat_buffer buffer;

    http::request<http::string_body> request;
    std::shared_ptr<void> response;

    HTTPRequestHandler* handler;
};

struct HTTPRequestHandler
{
    virtual void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) = 0;
};

#endif //CHATSNIFFER_HTTP_HTTPSESSION_H_
