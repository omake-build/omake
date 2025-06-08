#pragma once

#include <vector>
#include <string>
#include <utility>
#include <chrono>
#include <filesystem>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <regex>

namespace fs = std::filesystem;

namespace omake {

    struct BuildConfig {
        std::string outputName;
        std::string compiler;
        std::string makeTool;
        std::vector<std::string> buildOptions;
        std::vector<std::pair<std::string, std::string>> fileRules;
        bool showStats = false;
        bool useClBuild = false;
        std::string clPath;

        BuildConfig() : outputName("a.out"), compiler("g++"), makeTool("make") {}
    };

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

    class Parser {
    public:
        static BuildConfig parse(const std::string& filename);
    };

    class Builder {
    public:
        static void generateMakefile(const BuildConfig& config,
            const std::string& outputPath);
        static std::pair<size_t, size_t> getBuildStats(const std::string& makeOutput);

        static std::vector<std::string> convertOptionsForCl(
            const std::vector<std::string>& options);

    private:
        static void generateClMakefile(const BuildConfig& config,
            const std::string& outputPath,
            std::ofstream& makefile);
    };

    bool isClCompiler(const std::string& compiler);
    std::string convertToClOption(const std::string& option);

} // namespace omake