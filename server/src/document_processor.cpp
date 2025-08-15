#include "aio_server/document_processor.hpp"
#include <iostream>
#include <thread>
#include <memory>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>

DocumentProcessor::DocumentProcessor(std::string doc_path)
    : doc_path_(std::move(doc_path)) 
{}

void DocumentProcessor::process() {
    std::cout << "  -> [处理器 " << std::this_thread::get_id()
              << "] 开始处理文档: " << doc_path_ << std::endl;

    try {
        std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(doc_path_));
        if (!doc) {
            throw std::runtime_error("无法加载PDF文档: " + doc_path_);
        }

        std::string full_text_utf8;
        const int n = doc->pages();
        for (int i = 0; i < n; ++i) {
            std::unique_ptr<poppler::page> p(doc->create_page(i));
            if (!p) continue;

            poppler::ustring u = p->text();         // Unicode 文本
            auto bytes = u.to_utf8();               // std::vector<char>
            full_text_utf8.append(bytes.data(), bytes.size()); // ✅ 追加 UTF-8
        }

        std::cout << "  -> [处理器 " << std::this_thread::get_id()
                  << "] 成功提取文本，总计 " << full_text_utf8.size()
                  << " 字节（UTF-8）。" << std::endl;

        if (full_text_utf8.size() > 200) {
            std::cout << "      预览: " << full_text_utf8.substr(0, 200) << "..." << std::endl;
        } else {
            std::cout << "      内容: " << full_text_utf8 << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "  -> [处理器 " << std::this_thread::get_id()
                  << "] 处理文档时发生错误: " << e.what() << std::endl;
    }

    std::cout << "  -> [处理器 " << std::this_thread::get_id()
              << "] 文档处理完毕: " << doc_path_ << std::endl;
}

