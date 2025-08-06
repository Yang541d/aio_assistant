#include "aio_server/server.hpp"
#include <iostream>

int main() {
    try {
        boost::asio::io_context ioc;
        Server server(ioc, 8080); // 创建服务器
        server.run();             // 运行服务器
    } catch (const std::exception& e) {
        std::cerr << "服务器发生未捕获的异常: " << e.what() << std::endl;
    }
    return 0;
}