#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket);
    void start();

private:
    // 新的读写流程
    void do_read_header();
    void do_read_body();
    void handle_message();
    void do_write(nlohmann::json response_json);

    boost::asio::ip::tcp::socket socket_;
    
    // 用于读取消息头的缓冲区 (固定4字节)
    std::array<char, 4> header_buffer_;
    
    // 用于读取消息体的动态缓冲区
    std::vector<char> body_buffer_;

    // 存储从消息头解析出的消息体长度
    uint32_t incoming_body_length_ = 0;
};
