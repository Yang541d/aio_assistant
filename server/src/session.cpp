#include "aio_server/session.hpp"
#include "aio_server/document_processor.hpp" // ⭐新增⭐
#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

Session::Session(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket))
{}

void Session::start() {
    do_read_header();
}

void Session::do_read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(header_buffer_),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                const char* data = header_buffer_.data();
                incoming_body_length_ = 
                    (static_cast<uint32_t>(static_cast<uint8_t>(data[0])) << 24) |
                    (static_cast<uint32_t>(static_cast<uint8_t>(data[1])) << 16) |
                    (static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 8)  |
                    (static_cast<uint32_t>(static_cast<uint8_t>(data[3])));
                
                if (incoming_body_length_ > 0 && incoming_body_length_ < (1024 * 1024)) {
                    do_read_body();
                } else {
                    std::cerr << "收到了一个无效的消息长度: " << incoming_body_length_ << std::endl;
                    do_read_header();
                }
            } else if (ec != boost::asio::error::eof) {
                std::cerr << "读取消息头错误: " << ec.message() << std::endl;
            }
        });
}

void Session::do_read_body() {
    body_buffer_.resize(incoming_body_length_);
    auto self = shared_from_this();

    boost::asio::async_read(socket_, boost::asio::buffer(body_buffer_),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                handle_message();
            } else if (ec != boost::asio::error::eof) {
                std::cerr << "读取消息体错误: " << ec.message() << std::endl;
            }
        });
}

void Session::handle_message() {
    try {
        nlohmann::json request_json = nlohmann::json::parse(body_buffer_);
        std::cout << "[线程 " << std::this_thread::get_id() << "] 成功解析JSON: " 
                  << request_json.dump(4) << std::endl;

        nlohmann::json response_json;

        if (request_json.contains("command") && request_json["command"].is_string()) {
            std::string command = request_json["command"];

            if (command == "index_document") {
                if (request_json.contains("payload") && request_json["payload"].is_object() &&
                    request_json["payload"].contains("path") && request_json["payload"]["path"].is_string()) {
                    
                    std::string doc_path = request_json["payload"]["path"];
                    std::cout << "[I/O线程] 收到索引文档请求: " << doc_path << "。准备分派任务..." << std::endl;
                    
                    // ⭐ 核心改动：创建处理器并启动后台任务 ⭐
                    auto processor = std::make_shared<DocumentProcessor>(doc_path);
                    std::thread([processor] {
                        processor->process();
                    }).detach();

                    response_json["status"] = "task_received";
                    response_json["message"] = "Indexing task for '" + doc_path + "' has been accepted.";
                } else {
                    response_json["status"] = "error";
                    response_json["message"] = "Invalid payload for 'index_document' command.";
                }
            } else {
                response_json["status"] = "error";
                response_json["message"] = "Unknown command: " + command;
            }
        } else {
            response_json["status"] = "error";
            response_json["message"] = "Invalid request: 'command' field is missing or not a string.";
        }

        do_write(response_json);

    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON解析错误: " << e.what() << std::endl;
        do_read_header();
    }
}

void Session::do_write(nlohmann::json response_json) {
    auto self = shared_from_this();
    
    std::string response_body_str = response_json.dump();
    uint32_t body_length = response_body_str.length();

    auto write_buffer_ptr = std::make_shared<std::vector<char>>();
    write_buffer_ptr->resize(4 + body_length);

    (*write_buffer_ptr)[0] = (body_length >> 24) & 0xFF;
    (*write_buffer_ptr)[1] = (body_length >> 16) & 0xFF;
    (*write_buffer_ptr)[2] = (body_length >> 8) & 0xFF;
    (*write_buffer_ptr)[3] = body_length & 0xFF;

    std::copy(response_body_str.begin(), response_body_str.end(), write_buffer_ptr->begin() + 4);

    boost::asio::async_write(socket_, boost::asio::buffer(*write_buffer_ptr),
        [this, self, write_buffer_ptr](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                do_read_header();
            } else if (ec != boost::asio::error::eof) {
                std::cerr << "写入错误: " << ec.message() << std::endl;
            }
        });
}
