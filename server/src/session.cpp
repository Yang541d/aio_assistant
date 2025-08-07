#include "aio_server/session.hpp" 
#include <iostream>
#include <utility>
#include <istream>
using json = nlohmann::json;
// 构造函数的实现
Session::Session(boost::asio::ip::tcp::socket socket, Database& db)
    : socket_(std::move(socket)),
      db_(db) { // 初始化db_成员
}
void Session::start() {
    std::cout << "会话开始，准备从客户端读取数据..." << std::endl;
    do_read();
}

void Session::do_read() {
    auto self = shared_from_this();

    boost::asio::async_read_until(socket_, buffer_, '\n',
        [this, self](const boost::system::error_code& ec, std::size_t length) {
            if (!ec) {
                // 从streambuf中提取一行完整的数据
                std::istream is(&buffer_);
                std::string line;
                std::getline(is, line); // getline会自动处理换行符

                // 只在line非空时才尝试解析
                if (!line.empty()) {
                    try {
                        nlohmann::json request = nlohmann::json::parse(line);
                        std::cout << "成功解析JSON: " << request.dump(4) << std::endl;
                        handle_request(request);
                    } catch (const nlohmann::json::parse_error& e) {
                        std::cerr << "JSON解析错误: " << e.what() << std::endl;
                    }
                }

                // 继续准备下一次读取
                do_read();

            } else {
                if (ec == boost::asio::error::eof) {
                     std::cout << "客户端断开连接。" << std::endl;
                } else {
                     std::cerr << "读取数据时发生错误: " << ec.message() << std::endl;
                }
            }
        });
}

void Session::handle_request(const json& request) {
    db_.log_client_request(request);
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