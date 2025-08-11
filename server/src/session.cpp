#include "aio_server/session.hpp"
#include <iostream>
#include <utility>
#include <thread> // 为了打印线程ID

Session::Session(net::ip::tcp::socket&& socket, Database& db)
    : stream_(std::move(socket)),
      db_(db) {
}

void Session::start() {
    do_read();
}

void Session::do_read() {
    request_ = {}; // 清空上次的请求
    auto self = shared_from_this();

    http::async_read(stream_, buffer_, request_,
        [this, self](beast::error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);
            if (!ec) {
                handle_request(std::move(request_));
            } else if (ec != http::error::end_of_stream) {
                std::cerr << "read error: " << ec.message() << std::endl;
            }
        });
}

// 在 src/session.cpp 中
void Session::handle_request(http::request<http::string_body>&& req) {
    std::cout << "[线程 " << std::this_thread::get_id() << "] 收到HTTP请求: "
              << req.method_string() << " " << req.target() << std::endl;

    // --- API 路由开始 ---

    // 检查请求方法是否是 POST，并且请求路径(target)是否是 "/api/search"
    if (req.method() == http::verb::post && req.target() == "/api/search") {

        // 这是一个学者搜索请求，我们来处理它
        try {
            // 1. 解析请求体中的JSON
            nlohmann::json body_json = nlohmann::json::parse(req.body());

            // 2. 验证JSON中是否包含必须的 "name" 字段
            if (body_json.contains("name")) {
                std::string scholar_name = body_json["name"];
                std::cout << "API(/api/search): 收到搜索请求，学者姓名: " << scholar_name << std::endl;

                // TODO: 在这里创建任务，启动后台线程调用爬虫...

                // 3. 准备一个“202 Accepted”响应，表示任务已接受，正在后台处理
                http::response<http::string_body> res{http::status::accepted, req.version()};
                res.set(http::field::server, "AioServer/1.0");
                res.set(http::field::content_type, "application/json");

                nlohmann::json res_body;
                res_body["status"] = "pending";
                res_body["message"] = "Search task has been accepted for scholar: " + scholar_name;
                res_body["task_id"] = "temp-task-id-12345"; // 暂时用一个假的ID
                res.body() = res_body.dump(4); // dump(4)是格式化输出

                res.prepare_payload();
                send_response(std::move(res));
                return; // 处理完毕，直接返回，不再执行后续代码
            }
        } catch (const nlohmann::json::parse_error& e) {
            // 如果JSON解析失败，返回一个“400 Bad Request”客户端错误响应
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Invalid JSON format: " + std::string(e.what());
            res.prepare_payload();
            send_response(std::move(res));
            return;
        }
    }

    // --- 如果没有任何路由匹配，则返回404 Not Found ---

    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::content_type, "text/plain");
    res.body() = "The resource '" + std::string(req.target()) + "' was not found.";
    res.prepare_payload();
    send_response(std::move(res));
}

void Session::send_response(http::response<http::string_body>&& res) {
    auto self = shared_from_this();
    auto res_ptr = std::make_shared<http::response<http::string_body>>(std::move(res));

    http::async_write(stream_, *res_ptr,
        [this, self, res_ptr](beast::error_code ec, std::size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);
            if (ec) {
                std::cerr << "write error: " << ec.message() << std::endl;
            }
            // 简单处理，发送完响应就关闭连接
            beast::error_code close_ec;
            stream_.socket().shutdown(net::ip::tcp::socket::shutdown_send, close_ec);
        });
}