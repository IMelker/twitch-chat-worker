//
// Created by l2pic on 21.03.2021.
//

#ifndef CHATSNIFFER_HTTP_HTTPSESSION_H_
#define CHATSNIFFER_HTTP_HTTPSESSION_H_

#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

struct HTTPRequestHandler;
class HTTPSession : public std::enable_shared_from_this<HTTPSession>
{
  public:
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct SendLambda
    {
        explicit SendLambda(std::shared_ptr<HTTPSession> holder)
          : holder(std::move(holder)) {}

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            holder->response = sp;

            // Write the response
            http::async_write(holder->tcpStream, *sp,
                              beast::bind_front_handler(&HTTPSession::onWrite, holder, sp->need_eof()));
        }

        std::shared_ptr<HTTPSession> holder;
    };
  public:
    // Take ownership of the stream
    explicit HTTPSession(tcp::socket &&socket, HTTPRequestHandler *handler);
    ~HTTPSession();

    // Start the asynchronous operation
    void run();

    // Read request to object
    void read();
    void onRead(beast::error_code ec, std::size_t bytes);

    void onWrite(bool close, beast::error_code ec, std::size_t bytes);

    void close();
  private:
    beast::tcp_stream tcpStream;
    beast::flat_buffer inputBuffer;

    http::request<http::string_body> request;
    std::shared_ptr<void> response;

    HTTPRequestHandler* handler;
};

struct HTTPRequestHandler
{
    virtual void handleRequest(http::request<http::string_body> &&req, HTTPSession::SendLambda &&send) = 0;
};

#endif //CHATSNIFFER_HTTP_HTTPSESSION_H_
