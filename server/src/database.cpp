#include "aio_server/database.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <chrono>
#include <random>
#include <sstream>

Database::Database(const std::string& db_path) {
    try {
        db_ = std::make_unique<SQLite::Database>(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        std::cout << "数据库已打开: " << db_path << std::endl;

        // 开启外键支持, 对维护数据完整性很重要
        db_->exec("PRAGMA foreign_keys = ON;");

        // 创建系统事件表 (用于记录服务器启动等事件)
        db_->exec("CREATE TABLE IF NOT EXISTS system_events ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
                  "message TEXT NOT NULL"
                  ")");

        // 创建任务表
        db_->exec("CREATE TABLE IF NOT EXISTS tasks ("
                  "id TEXT PRIMARY KEY, task_type TEXT NOT NULL, status TEXT NOT NULL,"
                  "input_payload TEXT, created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

        // 创建学者候选人表 (将被新的scholars表替代，但作为过渡保留)
        db_->exec("CREATE TABLE IF NOT EXISTS search_candidates ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, task_id TEXT NOT NULL,"
                  "candidate_name TEXT NOT NULL, institution TEXT, profile_url_a TEXT UNIQUE,"
                  "avatar_url TEXT, FOREIGN KEY (task_id) REFERENCES tasks (id))");

        // 我们最新的scholars表设计
        db_->exec("CREATE TABLE IF NOT EXISTS scholars ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, aminer_id TEXT UNIQUE,"
                  "name TEXT NOT NULL, institution TEXT, profile_url_a TEXT,"
                  "avatar_url TEXT, status TEXT NOT NULL DEFAULT 'discovered',"
                  "task_id TEXT, FOREIGN KEY (task_id) REFERENCES tasks (id))");

    } catch (const std::exception& e) {
        std::cerr << "数据库初始化错误: " << e.what() << std::endl;
        throw; // 在构造函数中遇到严重错误时，最好抛出异常
    }
}

Database::~Database() {
    std::cout << "数据库已关闭。" << std::endl;
}

void Database::log_system_event(const std::string& event_message) {
    try {
        SQLite::Statement query(*db_, "INSERT INTO system_events (message) VALUES (?)");
        query.bind(1, event_message);
        query.exec();
    } catch (const std::exception& e) {
        std::cerr << "记录系统事件时发生错误: " << e.what() << std::endl;
    }
}

std::string Database::create_task(const std::string& task_type, const nlohmann::json& input) {
    // 生成一个简单的唯一ID (时间戳 + 随机数)
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1000, 9999);
    std::string task_id = "task-" + std::to_string(timestamp) + "-" + std::to_string(distrib(gen));
    
    try {
        SQLite::Transaction transaction(*db_);
        SQLite::Statement query(*db_, "INSERT INTO tasks (id, task_type, status, input_payload) VALUES (?, ?, ?, ?)");
        query.bind(1, task_id);
        query.bind(2, task_type);
        query.bind(3, "PENDING");
        query.bind(4, input.dump());
        query.exec();
        transaction.commit();
    } catch (const std::exception& e) {
        std::cerr << "创建任务时发生错误: " << e.what() << std::endl;
        return ""; // 返回空字符串表示失败
    }
    return task_id;
}

// 在 src/database.cpp 中
void Database::save_scholar_candidates(const std::string& task_id, const nlohmann::json& candidates) {
    try {
        SQLite::Transaction transaction(*db_);
        SQLite::Statement query(*db_, "INSERT OR IGNORE INTO scholars (aminer_id, name, institution, profile_url_a, avatar_url, task_id) VALUES (?, ?, ?, ?, ?, ?)");

        for (const auto& candidate : candidates) {
            // 在获取值之前，先检查是否为null
            std::string institution = candidate.contains("institution") && !candidate["institution"].is_null() 
                                        ? candidate["institution"].get<std::string>() 
                                        : ""; // 如果不存在或是null，则使用空字符串

            std::string avatar_url = candidate.contains("avatar_url") && !candidate["avatar_url"].is_null()
                                       ? candidate["avatar_url"].get<std::string>()
                                       : "";

            query.bind(1, candidate.value("id", ""));
            query.bind(2, candidate.value("name", ""));
            query.bind(3, institution);
            query.bind(4, candidate.value("urlA", ""));
            query.bind(5, avatar_url);
            query.bind(6, task_id);

            query.exec();
            query.reset();
        }
        transaction.commit();
        std::cout << "[数据库] 成功为任务 " << task_id << " 保存了 " << candidates.size() << " 条候选人记录。" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "保存候选人时发生错误: " << e.what() << std::endl;
    }
}

void Database::update_task_status(const std::string& task_id, const std::string& status) {
    try {
        SQLite::Statement query(*db_, "UPDATE tasks SET status = ? WHERE id = ?");
        query.bind(1, status);
        query.bind(2, task_id);
        query.exec();
    } catch (const std::exception& e) {
        std::cerr << "更新任务状态时发生错误: " << e.what() << std::endl;
    }
}