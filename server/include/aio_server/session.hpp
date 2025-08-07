#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "aio_server/database.hpp" 
#include <nlohmann/json.hpp> 



class Session : public std::enable_shared_from_this<Session> {
public:
    // 构造函数，接收一个已经建立的socket
    Session(boost::asio::ip::tcp::socket socket, Database& db);
    // 公共的启动函数
    void start();

private:
    void do_read();
    void handle_request(const nlohmann::json& request);
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf buffer_;
    // 保存Database的引用
    Database& db_;
};