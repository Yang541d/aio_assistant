#include "aio_server/session.hpp"
#include <iostream>
#include <utility>
#include <thread>
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
                // 将收到的请求移交给 handle_request 处理
                handle_request(std::move(request_));
            } else if (ec != http::error::end_of_stream) {
                std::cerr << "read error: " << ec.message() << std::endl;
            }
            // 当客户端主动断开连接时，会收到 end_of_stream 错误，此时会话自然结束
        });
}

void Session::handle_request(http::request<http::string_body>&& req) {
    std::cout << "[线程 " << std::this_thread::get_id() << "] 收到HTTP请求: "
              << req.method_string() << " " << req.target() << std::endl;

    // --- API 路由 ---
    if (req.method() == http::verb::post && req.target() == "/api/search") {
        try {
            nlohmann::json body_json = nlohmann::json::parse(req.body());
            if (!body_json.contains("name")) {
                // 如果缺少'name'字段，返回400错误
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.body() = "Missing 'name' field in request body.";
                res.prepare_payload();
                send_response(std::move(res));
                return;
            }

            // 1. 在数据库中创建任务
            std::string task_id = db_.create_task("SEARCH", body_json);
            if (task_id.empty()) {
                // 如果任务创建失败，返回500服务器内部错误
                http::response<http::string_body> res{http::status::internal_server_error, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.body() = "Failed to create a new task in database.";
                res.prepare_payload();
                send_response(std::move(res));
                return;
            }
            
            // 2. 启动后台线程执行长耗时任务
            std::thread([this, task_id, body_json] {
                db_.update_task_status(task_id, "RUNNING");
                std::cout << "[后台线程 " << std::this_thread::get_id() << "] 任务 " << task_id << " 开始执行..." << std::endl;
                
                try {
                    // 调用Python的 /search/scholars 接口
                    cpr::Response r = cpr::Post(cpr::Url{"http://127.0.0.1:8000/search/scholars"},
                                                cpr::Body{body_json.dump()},
                                                cpr::Header{{"Content-Type", "application/json"}});
                    
                    if (r.status_code == 200) {
                        nlohmann::json crawl_result = nlohmann::json::parse(r.text);
                        if (crawl_result.contains("items")) {
                            db_.save_scholar_candidates(task_id, crawl_result["items"]);
                            db_.update_task_status(task_id, "COMPLETED");
                        }
                    } else {
                        db_.update_task_status(task_id, "FAILED");
                        std::cerr << "[后台线程] 爬虫服务调用失败，任务 " << task_id << "，状态码: " << r.status_code << std::endl;
                    }
                } catch (const std::exception& e) {
                    db_.update_task_status(task_id, "FAILED");
                    std::cerr << "[后台线程] 任务 " << task_id << " 执行时发生异常: " << e.what() << std::endl;
                }
            }).detach();

            // 3. 立即向客户端返回“202 Accepted”响应
            http::response<http::string_body> res{http::status::accepted, req.version()};
            res.set(http::field::content_type, "application/json");
            nlohmann::json res_body;
            res_body["task_id"] = task_id;
            res_body["status"] = "PENDING";
            res_body["message"] = "Search task has been initiated.";
            res.body() = res_body.dump(4);
            res.prepare_payload();
            send_response(std::move(res));
            return; 
        } catch (const nlohmann::json::parse_error& e) {
             http::response<http::string_body> res{http::status::bad_request, req.version()};
             res.set(http::field::content_type, "text/plain");
             res.body() = "Invalid JSON format in request body.";
             res.prepare_payload();
             send_response(std::move(res));
             return;
        }
    }
    
    // 如果没有任何路由匹配，则返回404 Not Found
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
            beast::error_code close_ec;
            stream_.socket().shutdown(net::ip::tcp::socket::shutdown_send, close_ec);
        });
}