#include "aio_server/session.hpp"
#include <iostream>
#include <utility>
#include <thread> // 为了打印线程ID
#include <cpr/cpr.h>

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

    if (req.method() == http::verb::post && req.target() == "/api/search") {
        try {
            nlohmann::json body_json = nlohmann::json::parse(req.body());
            if (body_json.contains("name")) {
                std::string scholar_name = body_json["name"];
                std::cout << "API(/api/search): 收到搜索请求，学者姓名: " << scholar_name << std::endl;

                // ⭐核心改动：启动后台任务，并立即响应客户端⭐
                std::thread([this, scholar_name] {
                    std::cout << "[后台线程 " << std::this_thread::get_id() << "] 开始为 " << scholar_name << " 执行爬取任务..." << std::endl;

                    // 1. 调用Python爬虫服务 (这是阻塞操作)
                    cpr::Response r = cpr::Get(cpr::Url{"http://127.0.0.1:5001/crawl"}, cpr::Parameters{{"name", scholar_name}});

                    if (r.status_code == 200) {
                        try {
                            // 2. 解析爬虫返回的JSON结果
                            nlohmann::json crawl_result = nlohmann::json::parse(r.text);

                            // 3. 将结果写入数据库
                            if (crawl_result.contains("publications") && crawl_result["publications"].is_array()) {
                                db_.save_scholar_publications(scholar_name, crawl_result["publications"]);
                            }
                        } catch (const nlohmann::json::parse_error& e) {
                            std::cerr << "[后台线程 " << std::this_thread::get_id() << "] 解析爬虫结果时出错: " << e.what() << std::endl;
                        }
                    } else {
                        std::cerr << "[后台线程 " << std::this_thread::get_id() << "] 爬虫服务调用失败，状态码: " << r.status_code << std::endl;
                    }
                }).detach();

                // 立即发送“202 Accepted”响应
                http::response<http::string_body> res{http::status::accepted, req.version()};
                res.set(http::field::content_type, "application/json");
                nlohmann::json res_body;
                res_body["status"] = "pending";
                res_body["message"] = "Search task has been accepted for scholar: " + scholar_name;
                res_body["task_id"] = "temp-task-id-12345";
                res.body() = res_body.dump(4);
                res.prepare_payload();
                send_response(std::move(res));
                return; 
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