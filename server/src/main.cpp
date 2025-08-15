#include "aio_server/server.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <boost/asio.hpp> 

int main() {
    try {
        unsigned short const port = 8080;
        auto const threads = std::thread::hardware_concurrency();

        boost::asio::io_context ioc{static_cast<int>(threads)};

        std::cout << "V1服务器启动，监听端口 " << port 
                  << "，使用 " << threads << " 个工作线程..." << std::endl;
        
        Server server(ioc, port);
        server.start();
        
        std::vector<std::thread> thread_pool;
        thread_pool.reserve(threads);

        for (unsigned int i = 0; i < threads; ++i) {
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
