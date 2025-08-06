#include "aio_server/database.hpp"
#include <SQLiteCpp/SQLiteCpp.h> // 包含SQLiteCpp的头文件
#include <iostream>

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