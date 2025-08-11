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

void Session::handle_request(http::request<http::string_body>&& req) {
    std::cout << "[线程 " << std::this_thread::get_id() << "] 收到HTTP请求: "
              << req.method_string() << " " << req.target() << std::endl;

    // 在这里，我们将实现API路由
    // 目前，我们只返回一个简单的“OK”响应

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "AioServer/1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    nlohmann::json res_body;
    res_body["status"] = "ok";
    res_body["message"] = "Request received";
    res.body() = res_body.dump(4);

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