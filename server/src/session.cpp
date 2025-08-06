#include "aio_server/session.hpp" 
#include <iostream>
#include <utility>

// 构造函数的实现
Session::Session(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket)) {
}

void Session::start() {
    std::cout << "会话开始，准备从客户端读取数据..." << std::endl;
    do_read();
}

void Session::do_read() {
    auto self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(buffer_),
        [this, self](const boost::system::error_code& ec, std::size_t length) {
            if (!ec) {
                std::cout << "收到 " << length << " 字节数据，准备回响..." << std::endl;
                do_write(length);
            } else {
                std::cerr << "读取数据时发生错误: " << ec.message() << std::endl;
            }
        });
}

void Session::do_write(std::size_t length) {
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(buffer_, length),
        [this, self](const boost::system::error_code& ec, std::size_t /*length*/) {
            if (!ec) {
                do_read();
            } else {
                std::cerr << "写回数据时发生错误: " << ec.message() << std::endl;
            }
        });
}