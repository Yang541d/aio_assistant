#pragma once

#include "aio_server/database.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(net::ip::tcp::socket&& socket, Database& db);
    void start();

private:
    void do_read();
    void handle_request(http::request<http::string_body>&& req);
    void send_response(http::response<http::string_body>&& res);

    beast::tcp_stream stream_;
    Database& db_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
};