#include "aio_server/server.hpp"
#include "aio_server/session.hpp"
#include <iostream>
#include <memory>
#include <utility>

Server::Server(net::io_context& ioc, unsigned short port)
    : io_context_(ioc),
      acceptor_(ioc, {net::ip::tcp::v4(), port})
{
    db_ = std::make_unique<Database>("aio_assistant_data.db");
    db_->log_system_event("Server Initialized.");
}

void Server::start_accepting() {
    std::cout << "服务器核心：开始接受新连接..." << std::endl;
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](beast::error_code ec, net::ip::tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), *db_)->start();
            } else {
                std::cerr << "接受连接时发生错误: " << ec.message() << std::endl;
            }
            do_accept();
        });
}