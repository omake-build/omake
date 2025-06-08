#include "../include/omake.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
namespace omake {

    // CL编译器选项转换表
    static const std::unordered_map<std::string, std::string> clOptionsMap = {
        {"-Wall", "/W3"},         {"-Wextra", "/W4"},
        {"-Werror", "/WX"},       {"-O0", "/Od"},
        {"-O1", "/O1"},           {"-O2", "/O2"},
        {"-O3", "/Ox"},           {"-g", "/Zi"},
        {"-std=c++11", "/std:c++11"}, {"-std=c++14", "/std:c++14"},
        {"-std=c++17", "/std:c++17"}, {"-std=c++20", "/std:c++20"},
        {"-std=c++23", "/std:c++latest"}, {"-fPIC", ""},
        {"-shared", "/LD"},        {"-I", "/I"},
        {"-D", "/D"},              {"-l", ""}
    };

    // 实现 isClCompiler 函数
    bool isClCompiler(const std::string& compiler) {
        return compiler.find("cl.exe") != std::string::npos || compiler == "cl";
    }

    // 实现 convertToClOption 函数
    std::string convertToClOption(const std::string& option) {
        // 查找预设转换
        auto it = clOptionsMap.find(option);
        if (it != clOptionsMap.end()) {
            return it->second;
        }

        // 处理包含路径 (-Ipath)
        if (option.find("-I") == 0) {
            return "/I" + option.substr(2);
        }

        // 处理宏定义 (-Dmacro)
        if (option.find("-D") == 0) {
            return "/D" + option.substr(2);
        }

        // 处理链接库选项 (-llib)
        if (option.find("-l") == 0) {
            return option.substr(2) + ".lib";
        }

        // 其他未知选项保留原样
        return option;
    }

    // 实现 Builder::convertOptionsForCl 函数
    std::vector<std::string> Builder::convertOptionsForCl(
        const std::vector<std::string>& options) {
        std::vector<std::string> converted;

        for (const auto& opt : options) {
            std::string convertedOpt = convertToClOption(opt);
            if (!convertedOpt.empty()) {
                converted.push_back(convertedOpt);
            }
        }

        // 添加必要的CL基本库
        converted.push_back("kernel32.lib");
        converted.push_back("user32.lib");
        converted.push_back("gdi32.lib");

        return converted;
    }

    // 实现 Builder::generateClMakefile 函数
    void Builder::generateClMakefile(const BuildConfig& config,
        const std::string& outputPath,
        std::ofstream& makefile) {
        // 转换选项为CL兼容格式
        std::vector<std::string> clOptions;
        if (config.useClBuild) {
            clOptions = convertOptionsForCl(config.buildOptions);
        }
        else {
            clOptions = config.buildOptions;
        }

        // 编译器配置
        std::string clCompiler = config.clPath.empty() ? "cl.exe" : config.clPath;
        makefile << "CC = " << clCompiler << "\n\n";

        // 转换后的构建选项
        makefile << "CFLAGS = ";
        for (const auto& opt : clOptions) {
            makefile << opt << " ";
        }
        makefile << "\n\n";

        // 源文件收集
        makefile << "# 源文件列表\n";
        makefile << "SRCS = \\\n";
        for (const auto& rule : config.fileRules) {
            makefile << "    " << rule.first << " \\\n";
        }
        makefile << "\n\n";

        // 对象文件自动推导
        makefile << "# 对象文件列表\n";
        makefile << "OBJS = $(SRCS:.cpp=.obj)\n\n";

        // 目标配置
        makefile << "TARGET = " << config.outputName;
        if (config.outputName.find(".exe") == std::string::npos &&
            config.outputName.find(".dll") == std::string::npos &&
            config.outputName.find(".lib") == std::string::npos) {
            makefile << ".exe"; // 默认添加.exe后缀
        }
        makefile << "\n\n";

        // 主要构建规则
        makefile << "all: $(TARGET)\n\n";
        makefile << "$(TARGET): $(OBJS)\n";
        makefile << "\t$(CC) $(CFLAGS) /Fe:$@ $^\n\n";

        // 文件构建规则
        makefile << ".SUFFIXES: .cpp .obj\n\n";
        makefile << ".cpp.obj:\n";
        makefile << "\t$(CC) $(CFLAGS) /c $<\n\n";

        // 清理规则
        makefile << "clean:\n";
        makefile << "\tdel /Q $(TARGET) $(OBJS) *.ilk *.pdb *.exp 2>nul\n\n";

        // 伪目标声明
        makefile << ".PHONY: all clean\n";
    }

    // 实现 Builder::generateMakefile 函数
    void Builder::generateMakefile(const BuildConfig& config,
        const std::string& outputPath) {
        // 确保输出目录存在
        fs::create_directories(outputPath);

        std::ofstream makefile(outputPath + "/Makefile");

        if (config.useClBuild || isClCompiler(config.compiler)) {
            generateClMakefile(config, outputPath, makefile);
            return;
        }

        // 1. 添加进度报告（当启用统计时）
        if (config.showStats) {
            makefile << "# 进度报告\n";
            makefile << "PROGRESS := 0\n";
            makefile << "TOTAL := " << config.fileRules.size() << "\n\n";
        }

        // 2. 编译器配置
        makefile << "CC = " << config.compiler << "\n\n";

        // 3. 动态构建选项
        makefile << "CFLAGS = ";
        for (const auto& opt : config.buildOptions) {
            makefile << opt << " ";
        }
        makefile << "\n\n";

        // 4. 源文件收集（OBJS）
        makefile << "# 源文件列表\n";
        makefile << "SRCS = \\\n";
        for (const auto& rule : config.fileRules) {
            makefile << "    " << rule.first << " \\\n";
        }
        makefile << "\n\n";

        // 5. 对象文件自动推导
        makefile << "# 对象文件列表\n";
        makefile << "OBJS = $(addsuffix .o, $(basename $(notdir $(SRCS))))\n\n";

        // 6. 跨平台目标和工具配置
        makefile << "# 跨平台配置\n";
        makefile << "ifeq ($(OS),Windows_NT)\n";
        makefile << "    TARGET := " << config.outputName << ".exe\n";
        makefile << "    RM := del /Q\n";
        makefile << "    SOURCE_DIR := " << fs::absolute(".").string() << "\\\\\n";
        makefile << "else\n";
        makefile << "    TARGET := " << config.outputName << "\n";
        makefile << "    RM := rm -f\n";
        makefile << "    SOURCE_DIR := " << fs::absolute(".").string() << "/\n";
        makefile << "endif\n\n";

        // 7. 主要构建规则
        makefile << "all: $(TARGET)\n\n";
        makefile << "$(TARGET): $(OBJS)\n";
        // 添加进度报告
        if (config.showStats) {
            makefile << "\t@$(eval PROGRESS := $(PROGRESS) + $(words $?))\n";
            makefile << "\t@echo '[$(PROGRESS)/$(TOTAL)] Linking $@'\n";
        }
        makefile << "\t$(CC) $(CFLAGS) -o $@ $^\n\n";

        // 8. 文件构建规则
        for (const auto& rule : config.fileRules) {
            // 提取文件名
            fs::path sourcePath(rule.first);
            std::string fileName = sourcePath.filename().string();
            std::string objFile = fileName.substr(0, fileName.find_last_of('.')) + ".o";

            // 创建规则
            makefile << objFile << ": $(SOURCE_DIR)" << rule.first << " ";

            // 添加依赖的头文件（如果指定）
            if (!rule.second.empty()) {
                makefile << "$(SOURCE_DIR)" << rule.second << " ";
            }

            makefile << "\n";

            // 添加进度报告
            if (config.showStats) {
                makefile << "\t@$(eval PROGRESS := $(shell echo $$(($(PROGRESS)+1)))\n";
                makefile << "\t@echo '[$(PROGRESS)/$(TOTAL)] Building $@'\n";
            }

            makefile << "\t$(CC) $(CFLAGS) -c -o $@ $(SOURCE_DIR)" << rule.first << "\n\n";
        }

        // 9. 清理规则
        makefile << "clean:\n";
        makefile << "\t$(RM) $(TARGET) $(OBJS)\n\n";

        // 10. 伪目标声明
        makefile << ".PHONY: all clean\n";
    }

} // namespace omake