#pragma once
#include <boost/asio.hpp> 
#include <memory>

class Server {
public:
    Server(boost::asio::io_context& ioc, unsigned short port);
    void start();

private:
    void do_accept();

    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
};
