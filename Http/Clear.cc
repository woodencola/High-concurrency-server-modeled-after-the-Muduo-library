#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <chrono>
#include <set>

namespace fs = std::filesystem;

// 需要处理的文件扩展名（文本类，可能包含 URL）
const std::set<std::string> text_extensions = {
    ".html", ".htm", ".xhtml", ".php", ".asp", ".jsp",
    ".css", ".js", ".mjs", ".json", ".xml", ".xsl",
    ".txt", ".md", ".svg", ".csv", ".rss", ".atom"
};

// 判断文件是否为文本类型
bool is_text_file(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return text_extensions.find(ext) != text_extensions.end();
}

// 全局字符串替换（非正则，高效）
std::string replace_all(const std::string& input, const std::string& from, const std::string& to) {
    if (from.empty()) return input;
    std::string result;
    result.reserve(input.size());
    size_t pos = 0;
    size_t last_pos = 0;
    while ((pos = input.find(from, last_pos)) != std::string::npos) {
        result.append(input, last_pos, pos - last_pos);
        result.append(to);
        last_pos = pos + from.length();
    }
    result.append(input, last_pos, input.size() - last_pos);
    return result;
}

// 处理单个文件：读取内容，替换域名，写回
bool process_file(const fs::path& filepath,
                  const std::vector<std::pair<std::string, std::string>>& replacements,
                  bool backup) {
    // 备份（如果存在 .bak 且 backup=true，跳过备份）
    if (backup) {
        fs::path backup_path = filepath;
        backup_path += ".bak";
        if (!fs::exists(backup_path)) {
            try {
                fs::copy_file(filepath, backup_path, fs::copy_options::overwrite_existing);
            } catch (const std::exception& e) {
                std::cerr << "备份失败: " << filepath << " - " << e.what() << std::endl;
                return false;
            }
        }
    }

    // 读取原始内容（二进制方式，保留原样）
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "无法打开: " << filepath << std::endl;
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    // 执行替换
    std::string new_content = content;
    for (const auto& pair : replacements) {
        new_content = replace_all(new_content, pair.first, pair.second);
    }

    // 如果内容未变，跳过写入
    if (new_content == content) {
        return true;
    }

    // 写回
    std::ofstream ofs(filepath, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "无法写入: " << filepath << std::endl;
        return false;
    }
    ofs.write(new_content.data(), new_content.size());
    ofs.close();
    return true;
}

// 递归遍历目录
void walk_directory(const fs::path& root_dir,
                    const std::vector<std::pair<std::string, std::string>>& replacements,
                    bool backup) {
    size_t processed = 0, failed = 0, skipped = 0;
    auto start = std::chrono::steady_clock::now();

    for (const auto& entry : fs::recursive_directory_iterator(root_dir)) {
        if (!fs::is_regular_file(entry.status())) continue;

        if (!is_text_file(entry.path())) {
            ++skipped;
            continue;
        }

        // 显示正在处理的文件（相对路径）
        std::cout << "处理: " << fs::relative(entry.path(), root_dir).string() << std::endl;

        if (process_file(entry.path(), replacements, backup)) {
            ++processed;
        } else {
            ++failed;
        }
    }

    auto end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\n======== 处理完成 ========" << std::endl;
    std::cout << "成功处理: " << processed << " 个文件" << std::endl;
    std::cout << "失败: " << failed << " 个文件" << std::endl;
    std::cout << "跳过(非文本): " << skipped << " 个文件" << std::endl;
    std::cout << "耗时: " << elapsed << " 秒" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <目录路径> [新域名] [--backup]" << std::endl;
        std::cerr << "示例: " << argv[0] << " /home/ubuntu/galgamesource/www.ymgal.games" << std::endl;
        std::cerr << "示例(替换为IP+端口): " << argv[0] << " /path/to/site http://123.456.789.0:8080 --backup" << std::endl;
        std::cerr << "示例(协议相对): " << argv[0] << " /path/to/site //your-server.com" << std::endl;
        std::cerr << "说明: 不指定新域名则替换为空(变为绝对路径，如 /abc/def)" << std::endl;
        return 1;
    }

    fs::path root_dir = argv[1];
    if (!fs::exists(root_dir) || !fs::is_directory(root_dir)) {
        std::cerr << "错误: 目录不存在或不是目录" << std::endl;
        return 1;
    }

    std::string new_domain = "";
    bool backup = false;

    // 解析参数
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--backup") {
            backup = true;
        } else if (arg.find("http://") == 0 || arg.find("https://") == 0 || arg.find("//") == 0) {
            new_domain = arg;
        } else {
            std::cerr << "警告: 忽略未知参数 '" << arg << "'" << std::endl;
        }
    }

    // 定义需要替换的旧域名列表（原网站可能使用的各种形式）
    std::vector<std::pair<std::string, std::string>> replacements = {
        {"https://www.ymgal.games", new_domain},
        {"http://www.ymgal.games", new_domain},
        {"https://ymgal.games", new_domain},
        {"http://ymgal.games", new_domain},
        // 如果有其他子域名（如 cdn.ymgal.games）也可以添加
        {"https://cdn.ymgal.games", new_domain.empty() ? "" : (new_domain + "/cdn")},
    };

    std::cout << "开始处理目录: " << root_dir << std::endl;
    std::cout << "新域名: " << (new_domain.empty() ? "(空，即绝对路径)" : new_domain) << std::endl;
    std::cout << "备份模式: " << (backup ? "是 (原文件备份为 .bak)" : "否") << std::endl;
    std::cout << "========================================" << std::endl;

    walk_directory(root_dir, replacements, backup);

    return 0;
}