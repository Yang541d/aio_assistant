#pragma once
#include <string>

// 这是一个专门负责处理单个文档的类
class DocumentProcessor {
public:
    // 构造函数，接收要处理的文档路径
    explicit DocumentProcessor(std::string doc_path);

    // 公共的入口方法，启动处理流程
    void process();

private:
    std::string doc_path_;
};
