#include "aio_server/database.hpp"
#include "aio_server/server.hpp" 
#include "aio_server/session.hpp"
#include <iostream>
#include <memory>
#include <utility>
Server::Server(boost::asio::io_context& ioc, unsigned short port)
    : io_context_(ioc),
      acceptor_(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    // 创建数据库实例，数据库文件会生成在build目录下
    db_ = std::make_unique<Database>("aio_assistant_data.db");
    // 记录服务器启动事件
    db_->log_system_event("Server Started.");
}

void Server::run() {
    std::cout << "服务器启动，准备接受连接..." << std::endl;
    do_accept();
    io_context_.run();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "新客户端连接成功！创建会话处理..." << std::endl;
                std::make_shared<Session>(std::move(socket), *db_)->start();
            } else {
                std::cerr << "接受连接时发生错误: " << ec.message() << std::endl;
            }
            do_accept();
        });
}