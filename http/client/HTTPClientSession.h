//
// Created by l2pic on 16.05.2021.
//

#ifndef CHATCONTROLLER_HTTP_CLIENT_HTTPCLIENTSESSION_H_
#define CHATCONTROLLER_HTTP_CLIENT_HTTPCLIENTSESSION_H_

#include <memory>
#include <variant>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/strand.hpp>

#include "HTTPRequest.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class Logger;
class HTTPClientSession : public std::enable_shared_from_this<HTTPClientSession>
{
    using TCPStream = beast::tcp_stream;
    using SSLStream = beast::ssl_stream<beast::tcp_stream>;
  public:
    // Objects are constructed with a strand to
    // ensure that handlers do not execute concurrently.
    explicit HTTPClientSession(net::any_io_executor ex, Logger *logger);
    explicit HTTPClientSession(net::any_io_executor ex, ssl::context& ctx, Logger *logger);
    ~HTTPClientSession();

    // Start the asynchronous operation
    void run(HTTPRequest &&request);

    void close();

    void onResolve(beast::error_code ec, tcp::resolver::results_type results);
    void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void onHandshake(beast::error_code ec);
    void onWrite(beast::error_code ec, std::size_t bytes_transferred);
    void onRead(beast::error_code ec, std::size_t bytes_transferred);
    void onShutdown(beast::error_code ec);
  private:
    void logError(beast::error_code ec, std::string &&context);

    Logger *logger = nullptr;

    tcp::resolver resolver;
    std::variant<TCPStream, SSLStream> stream;
    beast::flat_buffer buffer; // (Must persist between reads)
    http::request<http::string_body> request;
    http::response<http::string_body> response;
};

#endif //CHATCONTROLLER_HTTP_CLIENT_HTTPCLIENTSESSION_H_
