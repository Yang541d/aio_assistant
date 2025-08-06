#pragma once
#include <boost/asio.hpp>
#include <memory> 
#include "aio_server/database.hpp" 
class Server {
public:
    Server(boost::asio::io_context& ioc, unsigned short port);
    void run();
private:
    void do_accept();

    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;

    std::unique_ptr<Database> db_; 
};