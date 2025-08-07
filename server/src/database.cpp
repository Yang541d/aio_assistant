#include "aio_server/database.hpp"
#include <SQLiteCpp/SQLiteCpp.h> // 包含SQLiteCpp的头文件
#include <iostream>

using json = nlohmann::json;

// 构造函数：打开数据库，并创建表（如果不存在）
Database::Database(const std::string& db_path) {
    try {
        // 使用智能指针创建数据库对象。
        // 参数是：文件名，以及打开模式（读写、如果不存在则创建）
        db_ = std::make_unique<SQLite::Database>(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        std::cout << "数据库已打开: " << db_path << std::endl;

        // 执行一条SQL语句，创建系统事件表
        // IF NOT EXISTS 可以确保这条语句在表已存在时不会报错
        db_->exec("CREATE TABLE IF NOT EXISTS system_events ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                  "message TEXT NOT NULL"
                  ")");

        // 创建客户端请求日志表
        db_->exec("CREATE TABLE IF NOT EXISTS client_requests ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "request_type TEXT,"
        "payload_json TEXT"
        ")");
} catch (const std::exception& e) {
        std::cerr << "数据库初始化错误: " << e.what() << std::endl;
        // 在实际项目中，这里应该抛出异常或处理错误
    }
}

// 析构函数在Database对象被销毁时自动调用，unique_ptr会在这里自动释放数据库连接
Database::~Database() {
    std::cout << "数据库已关闭。" << std::endl;
}

// 记录一条系统事件到数据库
void Database::log_system_event(const std::string& event_message) {
    try {
        // 准备一条SQL插入语句
        SQLite::Statement query(*db_, "INSERT INTO system_events (message) VALUES (?)");
        // 绑定参数。?号是占位符，这是防止SQL注入的安全做法
        query.bind(1, event_message);
        // 执行语句
        query.exec();
    } catch (const std::exception& e) {
        std::cerr << "记录系统事件时发生错误: " << e.what() << std::endl;
    }
}

// 在 src/database.cpp 中
void Database::log_client_request(const nlohmann::json& request) {
    // 我们用日志把这个函数包围起来，看看它是否被调用
    std::cout << "[DEBUG] ==> 进入 log_client_request 函数" << std::endl;
    try {
        std::cout << "[DEBUG] 准备创建 Transaction..." << std::endl;
        SQLite::Transaction transaction(*db_);
        std::cout << "[DEBUG] Transaction 已创建." << std::endl;

        std::cout << "[DEBUG] 准备创建 Statement..." << std::endl;
        SQLite::Statement query(*db_, "INSERT INTO client_requests (request_type, payload_json) VALUES (?, ?)");
        std::cout << "[DEBUG] Statement 已创建." << std::endl;

        std::string request_type = request.value("type", "unknown");
        std::string payload_str = request.value("payload", nlohmann::json::object()).dump();

        std::cout << "[DEBUG] 准备绑定参数..." << std::endl;
        query.bind(1, request_type);
        query.bind(2, payload_str);
        std::cout << "[DEBUG] 参数已绑定." << std::endl;

        std::cout << "[DEBUG] 准备执行 query.exec()..." << std::endl;
        query.exec();
        std::cout << "[DEBUG] query.exec() 执行完毕." << std::endl;

        std::cout << "[DEBUG] 准备提交 transaction.commit()..." << std::endl;
        transaction.commit();
        std::cout << "[DEBUG] transaction.commit() 执行完毕. 数据现在应该已存盘." << std::endl;

    } catch (const std::exception& e) {
        // 如果中间有任何错误，这里会打印出来
        std::cerr << "[ERROR] 在 log_client_request 中捕获到异常: " << e.what() << std::endl;
    }
    std::cout << "[DEBUG] <== 退出 log_client_request 函数" << std::endl;
}