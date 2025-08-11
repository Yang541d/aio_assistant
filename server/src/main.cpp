#include "aio_server/server.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <boost/asio.hpp>

namespace net = boost::asio;

int main() {
    try {
        unsigned short const port = 8080;
        auto const threads = 8;

        net::io_context ioc{threads};

        std::cout << "主服务器(server_app)启动，监听端口 " << port 
                  << "，使用 " << threads << " 个工作线程..." << std::endl;

        Server server(ioc, port);
        server.start_accepting();

        std::vector<std::thread> thread_pool;
        thread_pool.reserve(threads);

        for (int i = 0; i < threads; ++i) {
            thread_pool.emplace_back([&ioc] {
                ioc.run();
            });
        }

        for (auto& t : thread_pool) {
            if (t.joinable()) {
                t.join();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "服务器发生未捕获的异常: " << e.what() << std::endl;
    }
    return 0;
}