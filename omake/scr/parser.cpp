#include "..\include\omake.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>

namespace omake {

    BuildConfig Parser::parse(const std::string& filename) {
        BuildConfig config;
        std::ifstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("无法打开构建文件: " + filename);
        }

        std::string line;
        int lineNum = 0;
        // 支持单引号或双引号，支持1或2个参数
        std::regex pattern(R"((\w+)\s+(?:\"([^\"]+)\"|'([^']+)')(?:\s*,\s*(?:\"([^\"]+)\"|'([^']+)'))?\s*)");

        while (std::getline(file, line)) {
            lineNum++;
            // 跳过空行和注释 (允许行首空格)
            if (line.empty() || line.find_first_not_of(" \t") == std::string::npos ||
                line[line.find_first_not_of(" \t")] == '#') continue;

            std::smatch matches;
            if (std::regex_search(line, matches, pattern)) {
                // 显式转换为 std::string
                std::string cmd = matches[1].str();

                try {
                    if (cmd == "filebuild") {
                        // 检查子匹配有效性（组2或组3是第一个参数）
                        std::string source = matches[2].matched ? matches[2].str() : matches[3].str();

                        // 第二个参数可能在组4或组5
                        std::string header;
                        if (matches[4].matched) header = matches[4].str();
                        else if (matches[5].matched) header = matches[5].str();

                        config.fileRules.emplace_back(source, header);
                    }
                    else if (cmd == "buildfile") {
                        // 使用组2或组3作为参数
                        config.outputName = matches[2].matched ? matches[2].str() : matches[3].str();
                    }
                    else if (cmd == "buildcode") {
                        config.compiler = matches[2].matched ? matches[2].str() : matches[3].str();
                    }
                    else if (cmd == "buildmake") {
                        config.makeTool = matches[2].matched ? matches[2].str() : matches[3].str();
                    }
                    else if (cmd == "building") {
                        config.buildOptions.push_back(
                            matches[2].matched ? matches[2].str() : matches[3].str()
                        );
                    }
                    // 注意：这里不需要处理 --starts，它是命令行选项
                }
                catch (...) {
                    throw std::runtime_error("解析错误 [行 " + std::to_string(lineNum) + "]");
                }
            }
        }

        // 验证源文件列表
        if (config.fileRules.empty()) {
            throw std::runtime_error("没有指定任何源文件(filebuild)");
        }

        return config;
    }

} // namespace omake