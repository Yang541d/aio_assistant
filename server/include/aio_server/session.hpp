#pragma once

#include <memory>
#include <boost/asio.hpp>

class Session : public std::enable_shared_from_this<Session> {
public:
    // 构造函数，接收一个已经建立的socket
    Session(boost::asio::ip::tcp::socket socket);
    // 公共的启动函数
    void start();

private:
    void do_read();
    void do_write(std::size_t length);

    boost::asio::ip::tcp::socket socket_;
    std::array<char, 1024> buffer_;
};