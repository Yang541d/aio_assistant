#include "aio_server/session.hpp" 
#include <iostream>
#include <utility>
using json = nlohmann::json;
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
                try {
                    // 将收到的数据转换成一个json对象
                    // 注意：buffer_是一个字符数组，但可能不包含字符串结束符'\0'
                    // 所以我们用(buffer_.data(), length)来构造一个只包含有效数据的视图
                    json request = json::parse(buffer_.data(), buffer_.data() + length);

                    std::cout << "成功解析JSON: " << request.dump(4) << std::endl; // dump(4)是格式化打印

                    // 调用新的处理函数
                    handle_request(request);

                } catch (const json::parse_error& e) {
                    std::cerr << "JSON解析错误: " << e.what() << std::endl;
                    // TODO: 可以向客户端发送一个错误信息
                }

                // 无论成功与否，都继续准备下一次读取
                do_read();

            } else {
                // 当客户端断开连接时，会触发一个错误，通常是 "End of file"
                // 这不是一个严重错误，只是表示会话正常结束
                if (ec == boost::asio::error::eof) {
                     std::cout << "客户端断开连接。" << std::endl;
                } else {
                     std::cerr << "读取数据时发生错误: " << ec.message() << std::endl;
                }
            }
        });
}

void Session::handle_request(const json& request) {
    // 检查JSON中是否存在"type"字段
    if (request.contains("type") && request["type"].is_string()) {
        std::string request_type = request["type"];

        if (request_type == "echo") {
            std::cout << "收到一个echo请求。" << std::endl;
            // 在这里我们可以实现一个JSON版本的echo，但暂时先只打印
        } else if (request_type == "login") {
            if (request.contains("payload") && request["payload"].is_object()) {
                auto payload = request["payload"];
                if (payload.contains("user")) {
                    std::string user = payload["user"];
                    std::cout << "收到来自用户 " << user << " 的登录请求。" << std::endl;
                }
            }
        } else {
            std::cout << "收到未知类型的请求: " << request_type << std::endl;
        }
    }
}

// void Session::do_write(std::size_t length) {
//     auto self = shared_from_this();
//     boost::asio::async_write(socket_, boost::asio::buffer(buffer_, length),
//         [this, self](const boost::system::error_code& ec, std::size_t /*length*/) {
//             if (!ec) {
//                 do_read();
//             } else {
//                 std::cerr << "写回数据时发生错误: " << ec.message() << std::endl;
//             }
//         });
// }