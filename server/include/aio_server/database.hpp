#pragma once

#include <string>
#include <memory>

namespace SQLite { class Database; }

class Database {
public:
    Database(const std::string& db_path);
    ~Database();

    // 记录一条系统事件
    void log_system_event(const std::string& event_message);

private:
    // 使用智能指针来管理数据库对象，确保资源被正确释放
    std::unique_ptr<SQLite::Database> db_;
};