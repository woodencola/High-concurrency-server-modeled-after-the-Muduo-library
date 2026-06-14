#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>
#include <algorithm>

// ========== RewriteHtmlLinks 函数（使用你已有的逻辑） ==========
static std::string RewriteHtmlLinks(const std::string& html,
                                    const std::string& target_domain,
                                    const std::string& local_prefix = "/",
                                    const std::vector<std::string>& exclude_extensions = {".jpg", ".jpeg", ".png", ".gif", ".webp", ".svg", ".ico", ".mp4", ".webm"}) {
    static std::regex url_regex(R"((href|src)=\"(https?://[^\"]+)\")");
    std::string result;
    auto it = std::sregex_iterator(html.begin(), html.end(), url_regex);
    auto end = std::sregex_iterator();
    size_t last_pos = 0;
    for (; it != end; ++it) {
        const std::smatch& match = *it;
        result.append(html, last_pos, match.position() - last_pos);
        std::string attr = match[1].str();   // "href" 或 "src"
        std::string url = match[2].str();    // 完整 URL

        bool need_replace = (url.find(target_domain) == 0);
        if (need_replace) {
            bool excluded = false;
            for (const auto& ext : exclude_extensions) {
                if (url.length() > ext.length() && url.substr(url.length() - ext.length()) == ext) {
                    excluded = true;
                    break;
                }
            }
            if (!excluded) {
                // 提取路径部分：去掉域名，保留 /path?query...
                std::string path = url.substr(target_domain.length());
                if (path.empty()) path = "/";
                std::string new_url = local_prefix + path;
                result += attr + "=\"" + new_url + "\"";
            } else {
                result += attr + "=\"" + url + "\"";
            }
        } else {
            result += attr + "=\"" + url + "\"";
        }
        last_pos = match.position() + match.length();
    }
    result.append(html, last_pos, html.size() - last_pos);
    return result;
}

// ========== 工具函数：读取文件内容 ==========
bool ReadFile(const std::string& filename, std::string& content) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    content.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();
    return true;
}

// ========== 工具函数：写入文件 ==========
bool WriteFile(const std::string& filename, const std::string& content) {
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "无法写入文件: " << filename << std::endl;
        return false;
    }
    ofs.write(content.data(), content.size());
    ofs.close();
    return true;
}

int main(int argc, char* argv[]) {
    // 默认路径（你可以修改，或者通过命令行参数传入）
    std::string input_path = "/home/ubuntu/High-concurrency-server-modeled-after-the-Muduo-library/Http/index.html";
    std::string output_path = "/home/ubuntu/High-concurrency-server-modeled-after-the-Muduo-library/Http/index_rewritten.html";

    // 如果命令行给了参数，则使用第一个参数作为输入文件
    if (argc > 1) {
        input_path = argv[1];
    }
    if (argc > 2) {
        output_path = argv[2];
    }

    std::string html;
    if (!ReadFile(input_path, html)) {
        return 1;
    }

    std::cout << "读取文件成功，原始 HTML 大小: " << html.size() << " 字节" << std::endl;

    // 配置重写参数（根据你的实际镜像网站修改）
    std::string target_domain = "https://www.ymgal.games";   // 要替换的原始域名
    std::string local_prefix = "/home/ubuntu/galgamesource/www.ymgal.games";                           // 本地前缀（你的服务器根路径）
    std::vector<std::string> exclude_ext = {".jpg", ".jpeg", ".png", ".gif", ".webp", ".svg", ".ico", ".mp4", ".webm"};

    std::string rewritten = RewriteHtmlLinks(html, target_domain, local_prefix, exclude_ext);

    // 输出到终端（前 2000 个字符预览）
    std::cout << "\n========== 重写后的 HTML 预览（前 2000 字符） ==========" << std::endl;
    std::cout << rewritten.substr(0, 2000) << std::endl;
    if (rewritten.size() > 2000) {
        std::cout << "... (省略剩余 " << (rewritten.size() - 2000) << " 字符)" << std::endl;
    }

    // 保存到文件
    if (WriteFile(output_path, rewritten)) {
        std::cout << "\n重写后的完整 HTML 已保存至: " << output_path << std::endl;
    }

    // 简单验证
    bool still_has_domain = rewritten.find(target_domain) != std::string::npos;
    std::cout << "\n验证结果: " << (still_has_domain ? "❌ 页面中仍包含原域名，重写未完全生效" : "✅ 原域名已被完全替换") << std::endl;

    return 0;
}