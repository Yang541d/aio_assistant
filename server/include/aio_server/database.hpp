#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace SQLite { class Database; }

class Database {
public:
    Database(const std::string& db_path);
    ~Database();

    void log_system_event(const std::string& event_message);

    // 创建一个搜索任务
    std::string create_task(const std::string& task_type, const nlohmann::json& input);

    // 保存学者候选人列表
    void save_scholar_candidates(const std::string& task_id, const nlohmann::json& candidates);

    // 更新任务状态
    void update_task_status(const std::string& task_id, const std::string& status);

private:
    std::unique_ptr<SQLite::Database> db_;
};