#include "../include/omake.h"
#include <filesystem>
#include <iostream>
#include <thread>
#include <cstdio>
#include <atomic>

namespace fs = std::filesystem;

namespace {
    // 安全的缓冲区读取函数
    std::string readBuffer(FILE* pipe) {
        constexpr size_t BUFFER_SIZE = 128;
        char buffer[BUFFER_SIZE];
        std::string result;

        while (!feof(pipe)) {
            if (fgets(buffer, BUFFER_SIZE, pipe) != nullptr) {
                result += buffer;
            }
        }

        return result;
    }
}

int main(int argc, char* argv[]) {
    std::string sourceDir = ".";
    std::string outputDir = "out";
    std::string buildFile = "omakefile.txt";
    bool showStats = false;
    bool useClBuild = false;
    std::string clPath;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--starts") {
            showStats = true;
        }
        else if (arg == "--clbuild") {
            useClBuild = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                clPath = argv[++i];
            }
        }
        else if (arg == "-build" && i + 1 < argc) {
            sourceDir = argv[++i];
        }
        else if (arg == "-B" && i + 1 < argc) {
            outputDir = argv[++i];
        }
        else if (arg == "-f" && i + 1 < argc) {
            buildFile = argv[++i];
        }
    }

    try {
        // 构建完整的构建文件路径
        auto buildFilePath = fs::path(sourceDir) / buildFile;

        // 设置当前工作目录
        fs::current_path(sourceDir);

        // 解析构建文件
        auto config = omake::Parser::parse(buildFilePath.filename().string());
        config.showStats = showStats;
        config.useClBuild = useClBuild;
        config.clPath = clPath;

        // 如果启用了CL构建，覆盖编译器设置
        if (useClBuild) {
            if (clPath.empty()) {
                config.compiler = "cl.exe";
            }
            else {
                config.compiler = clPath;
            }
        }

        // 生成Makefile
        omake::Builder::generateMakefile(config, outputDir);

        // 获取正确的 make 命令
        std::string makeCommand = config.makeTool;

        // 跨平台执行构建命令
#if defined(_WIN32)
        if (makeCommand == "make") {
            makeCommand = "mingw32-make";
        }

        std::string command = makeCommand + " -C \"" +
            fs::absolute(outputDir).string() + "\"";

        // 添加统计选项
        if (showStats) {
            command += " --always-make --output-sync=target --no-print-directory";
        }
#else
        std::string command = makeCommand + " -C \"" + outputDir + "\"";

        // 添加统计选项
        if (showStats) {
            command += " --always-make --output-sync=target --no-print-directory";
        }
#endif

        // 检查CL环境
        if (useClBuild) {
            const char* vcvars = std::getenv("VCINSTALLDIR");
            if (vcvars == nullptr) {
                std::cerr << "警告: 未检测到Visual Studio环境，CL构建可能失败\n";
                std::cerr << "请确保在'开发者命令提示符'中运行此命令\n";
            }
        }

        // 准备构建统计
        omake::BuildStats stats;
        size_t totalFiles = config.fileRules.size();

        // 执行命令并捕获输出
        if (showStats) {
            stats.start();

#if defined(_WIN32)
            FILE* pipe = _popen(command.c_str(), "r");
#else
            FILE* pipe = popen(command.c_str(), "r");
#endif
            if (!pipe) {
                throw std::runtime_error("无法执行命令: " + command);
            }

            // 用于线程同步的原子标志
            std::atomic<bool> keepRunning(true);

            // 启动线程用于显示进度条
            std::thread progressThread([&]() {
                while (keepRunning) {
                    // 获取当前输出
                    std::string output = readBuffer(pipe);

                    // 解析进度
                    auto [builtFiles, outputTotal] = omake::Builder::getBuildStats(output);
                    if (outputTotal == 0) outputTotal = totalFiles;

                    // 更新统计
                    stats.update(builtFiles, outputTotal);

                    // 显示进度条和统计信息
                    std::cout << "\r" << stats.getProgressBar() << " " << stats.getSummary() << std::flush;

                    // 短暂睡眠以避免过度占用CPU
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                });

            // 等待进程完成
#if defined(_WIN32)
            int result = _pclose(pipe);
#else
            int result = pclose(pipe);
#endif
            keepRunning = false;
            progressThread.join();

            // 输出最终结果
            std::cout << "\n构建" << (result == 0 ? "成功" : "失败")
                << "，共 " << totalFiles << " 个文件\n";

            if (result != 0) {
                return 1;
            }
        }
        else {
            // 不显示统计信息，直接执行
            std::cout << "执行: " << command << std::endl;
            int result = std::system(command.c_str());
            if (result != 0) {
                std::cerr << "构建失败，退出代码: " << result << std::endl;
                return 1;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\n错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}