#include "aio_server/server.hpp"
#include "aio_server/session.hpp"
#include <iostream>
#include <utility>

Server::Server(boost::asio::io_context& ioc, unsigned short port)
    : ioc_(ioc),
      acceptor_(ioc, {boost::asio::ip::tcp::v4(), port})
{}

void Server::start() {
    std::cout << "服务器核心：开始接受新连接..." << std::endl;
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
      
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket))->start();
            } else {
                std::cerr << "接受连接时发生错误: " << ec.message() << std::endl;
            }
            do_accept();
        });
}
