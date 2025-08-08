#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>

// 定义命名空间的别名
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// 处理单个HTTP会话的类
class HttpSession : public std::enable_shared_from_this<HttpSession> {
private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;

public:
    // 构造函数，接收一个已经建立的socket
    HttpSession(tcp::socket&& socket) : stream_(std::move(socket)) {}

    void run() {
        do_read();
    }

private:
    void do_read() {
        // 清空请求对象，为下一次请求做准备
        req_ = {};

        // 从流中异步读取一个HTTP请求
        http::async_read(stream_, buffer_, req_,
            [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
                boost::ignore_unused(bytes_transferred);
                if (!ec) {
                    // 读取成功，处理请求
                    self->handle_request();
                } else {
                    std::cerr << "read error: " << ec.message() << std::endl;
                }
            });
    }

        // 在 HttpSession 类中
    void handle_request() {
        // res_ptr 这个智能指针现在负责管理我们响应对象的生命周期
        auto res_ptr = std::make_shared<http::response<http::string_body>>(http::status::ok, req_.version());

        // 现在我们通过指针来修改响应对象
        res_ptr->set(http::field::server, "AioServer/1.0");
        res_ptr->set(http::field::content_type, "text/plain");
        res_ptr->keep_alive(req_.keep_alive());
        res_ptr->body() = "Hello, Boost.Beast! (Fixed)"; // 我们在内容里加个标记
        res_ptr->prepare_payload();

        // 我们将 res_ptr 捕获到回调函数中
        http::async_write(stream_, *res_ptr, // 注意，async_write函数需要传入对象本身，而不是指针
            [self = shared_from_this(), res_ptr](beast::error_code ec, std::size_t bytes_transferred) {
                // 因为回调函数持有了res_ptr的一份拷贝，
                // 它可以保证在回调函数执行完毕前，我们堆上的响应对象绝对不会被销毁。
                boost::ignore_unused(bytes_transferred);
                if (ec) {
                    std::cerr << "write error: " << ec.message() << std::endl;
                }
                
                // 为shutdown操作创建一个新的、干净的error_code对象
                beast::error_code shutdown_ec;
                self->stream_.socket().shutdown(tcp::socket::shutdown_send, shutdown_ec);
            });
    }
};

// 负责监听和接受新连接的类
class Listener : public std::enable_shared_from_this<Listener> {
private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    Listener(net::io_context& ioc, unsigned short port)
        : ioc_(ioc), acceptor_(ioc, {tcp::v4(), port}) {}

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // 创建一个HttpSession来处理这个新连接
                    std::make_shared<HttpSession>(std::move(socket))->run();
                }
                // 继续等待下一个连接
                self->do_accept();
            });
    }
};

int main() {
    try {
        unsigned short const port = 8080;
        net::io_context ioc{1}; // 构造函数参数1表示使用1个线程

        std::cout << "HTTP服务器启动，监听端口 " << port << "..." << std::endl;

        // 创建并运行Listener
        auto listener = std::make_shared<Listener>(ioc, port);
        listener->run(); // 现在调用run()是安全的

        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}