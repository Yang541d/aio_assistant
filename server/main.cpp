#include <iostream>
#include <memory> // 用于std::shared_ptr
#include <boost/asio.hpp>

void do_accept(boost::asio::ip::tcp::acceptor& acceptor);

int main() {
    try {
        boost::asio::io_context ioc;

        boost::asio::ip::tcp::acceptor acceptor(ioc,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8080));

        std::cout << "服务器启动，监听 8080 端口..." << std::endl;

        do_accept(acceptor);

        ioc.run();
    }
    catch (const std::exception& e) {
        std::cerr << "服务器发生未捕获的异常: " << e.what() << std::endl;
    }

    return 0;
}

void do_accept(boost::asio::ip::tcp::acceptor& acceptor) {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor.get_executor());

    acceptor.async_accept(*socket,
        [&acceptor, socket](const boost::system::error_code& error) {
            if (!error) {
                std::cout << "客户端连接成功！来自: " 
                          << socket->remote_endpoint().address().to_string() 
                          << std::endl;
            }

            do_accept(acceptor);
        }
    );
}