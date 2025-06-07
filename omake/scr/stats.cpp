#include "..\include\omake.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <regex>

namespace omake {

    void BuildStats::start() {
        startTime = std::chrono::steady_clock::now();
    }

    void BuildStats::update(size_t builtFiles, size_t totalFiles) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() / 1000.0;

        if (elapsed > 0.1) {
            // 更新速度，文件/秒
            lastSpeed = builtFiles / elapsed;
            lastBuiltFiles = builtFiles;
        }
    }

    std::string BuildStats::getProgressBar(size_t width) const {
        const double progress = static_cast<double>(lastBuiltFiles) / width;
        const size_t pos = static_cast<size_t>(width * progress);

        std::string progressBar = "[";
        progressBar.reserve(width + 10); // 预留空间

        for (size_t i = 0; i < width; ++i) {
            if (i < pos) progressBar += "=";
            else if (i == pos) progressBar += ">";
            else progressBar += " ";
        }
        progressBar += "]";

        return progressBar;
    }

    std::string BuildStats::getSummary() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() / 1000.0;

        std::stringstream ss;
        // 修复中文乱码问题（使用ASCII字符）
        ss << "Built: " << lastBuiltFiles << " files";
        ss << " | Time: " << std::fixed << std::setprecision(1) << elapsed << "s";
        ss << " | Speed: " << std::fixed << std::setprecision(1) << lastSpeed << " files/s";

        return ss.str();
    }

    // 修改统计信息解析逻辑
    std::pair<size_t, size_t> Builder::getBuildStats(const std::string& makeOutput) {
        size_t builtFiles = 0;
        size_t totalFiles = 0;

        // 改进正则表达式匹配进度格式
        static std::regex progress_re(R"(\[(\d+)/(\d+)\])");
        std::smatch matches;
        
        if (std::regex_search(makeOutput, matches, progress_re)) {
            try {
                builtFiles = std::stoul(matches[1]);
                totalFiles = std::stoul(matches[2]);
            }
            catch (...) {
                // 忽略转换错误
            }
        }
        
        return { builtFiles, totalFiles };
    }

} // namespace omake