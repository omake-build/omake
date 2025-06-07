#pragma once
#include <vector>
#include <string>
#include <utility>
#include <chrono> // 添加计时支持
#include <filesystem>

namespace fs = std::filesystem;

namespace omake {

    struct BuildConfig {
        std::string outputName;
        std::string compiler;
        std::string makeTool;
        std::vector<std::string> buildOptions;
        std::vector<std::pair<std::string, std::string>> fileRules;
        bool showStats = false;  // 新增：是否显示统计信息

        BuildConfig() : outputName("a.out"), compiler("g++"), makeTool("make") {}
    };

    class Parser {
    public:
        static BuildConfig parse(const std::string& filename);
    };

    class Builder {
    public:
        static void generateMakefile(const BuildConfig& config,
            const std::string& outputPath);

        // 新增：获取构建统计信息
        static std::pair<size_t, size_t> getBuildStats(const std::string& makeOutput);
    };

    // 新增：进度统计类
    class BuildStats {
    public:
        void start();
        void update(size_t builtFiles, size_t totalFiles);
        std::string getProgressBar(size_t width = 50) const;
        std::string getSummary() const;

    private:
        std::chrono::steady_clock::time_point startTime;
        size_t lastBuiltFiles = 0;
        double lastSpeed = 0.0;
    };

} // namespace omake