//
// Created by imelker on 21.03.2021.
//

#ifndef CHATSNIFFER_HTTP_HTTPLISTENER_H_
#define CHATSNIFFER_HTTP_HTTPLISTENER_H_

#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class Logger;
struct HTTPRequestHandler;

// Accepts incoming connections and launches the sessions
class HTTPListener : public std::enable_shared_from_this<HTTPListener>
{
  public:
    HTTPListener(net::io_context &ioc, tcp::endpoint endpoint, std::shared_ptr<Logger> logger, HTTPRequestHandler *handler);
    ~HTTPListener();

    void startAcceptConnections();
  private:
    void accept();
    void onAccept(beast::error_code ec, tcp::socket socket);

    std::shared_ptr<Logger> logger;

    net::io_context& ioc;
    tcp::endpoint endpoint;
    tcp::acceptor acceptor;
    HTTPRequestHandler *handler;
};

#endif //CHATSNIFFER_HTTP_HTTPLISTENER_H_
