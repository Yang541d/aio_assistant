#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
namespace SQLite { class Database; }

class Database {
public:
    Database(const std::string& db_path);
    ~Database();

    // 记录一条系统事件
    void log_system_event(const std::string& event_message);
    // 记录一次客户端请求的方法
    void log_client_request(const nlohmann::json& request);
    // ⭐新增⭐ 保存学者论文信息的方法
    void save_scholar_publications(const std::string& scholar_name, const nlohmann::json& publications);


private:
    // 使用智能指针来管理数据库对象，确保资源被正确释放
    std::unique_ptr<SQLite::Database> db_;
};