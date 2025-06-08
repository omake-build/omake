#include "../include/omake.h"

namespace omake {

    void BuildStats::start() {
        startTime = std::chrono::steady_clock::now();
    }

    void BuildStats::update(size_t builtFiles, size_t totalFiles) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() / 1000.0;

        if (elapsed > 0.1) {
            lastSpeed = builtFiles / elapsed;
            lastBuiltFiles = builtFiles;
        }
    }

    std::string BuildStats::getProgressBar(size_t width) const {
        const double progress = static_cast<double>(lastBuiltFiles) / width;
        const size_t pos = static_cast<size_t>(width * progress);

        std::string progressBar = "[";
        progressBar.reserve(width + 10);

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
        ss << "已构建: " << lastBuiltFiles << " 个文件";
        ss << " | 用时: " << std::fixed << std::setprecision(1) << elapsed << " 秒";
        ss << " | 速度: " << std::fixed << std::setprecision(1) << lastSpeed << " 文件/秒";

        return ss.str();
    }

    std::pair<size_t, size_t> Builder::getBuildStats(const std::string& makeOutput) {
        size_t builtFiles = 0;
        size_t totalFiles = 0;

        // 查找进度信息：格式为 [XX/YY]
        size_t startPos = makeOutput.find('[');
        while (startPos != std::string::npos) {
            size_t endPos = makeOutput.find(']', startPos);
            if (endPos != std::string::npos) {
                std::string progress = makeOutput.substr(startPos + 1, endPos - startPos - 1);

                // 分离已构建和总数
                size_t slashPos = progress.find('/');
                if (slashPos != std::string::npos) {
                    try {
                        builtFiles = std::stoul(progress.substr(0, slashPos));
                        totalFiles = std::stoul(progress.substr(slashPos + 1));
                    }
                    catch (...) {
                        // 忽略转换错误
                    }
                }
            }
            startPos = makeOutput.find('[', endPos + 1);
        }

        return { builtFiles, totalFiles };
    }

} // namespace omake