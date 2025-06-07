#include "..\include\omake.h"
#include <iostream>
#include <filesystem>
#include <thread>   // 用于延迟
#include <cstdio>   // 用于 popen
#include <atomic>   // 用于线程同步
// 在文件头部添加
#ifdef _WIN32
    #include <process.h>
#endif
#if defined(_WIN32)
    #define popen _popen
    #define pclose _pclose
#endif
namespace fs = std::filesystem;

namespace {
    // 修改后的安全缓冲区读取函数（逐行读取）
    std::string readBuffer(FILE* pipe) {
        constexpr size_t BUFFER_SIZE = 128;
        char buffer[BUFFER_SIZE];
        std::string result;
        
        // 修改为逐行非阻塞读取
        while (fgets(buffer, BUFFER_SIZE, pipe) != nullptr) {
            result += buffer;
            break; // 每次只读取一行
        }
        
        return result;
    }
}

int main(int argc, char* argv[]) {
    std::string sourceDir = ".";
    std::string outputDir = "out";
    std::string buildFile = "omakefile.txt";
    bool showStats = false; // 默认不显示统计

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stats") {
            showStats = true;
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
        config.showStats = showStats; // 设置统计标志

        // 生成Makefile
        omake::Builder::generateMakefile(config, outputDir);

        // 获取正确的 make 命令
        std::string makeCommand = config.makeTool;

        // 跨平台执行构建命令
#if defined(_WIN32)
    // Windows 默认使用 MinGW make
        if (makeCommand == "make") {
            makeCommand = "mingw32-make";
        }

        // 使用绝对路径避免 cd 问题
        std::string command = makeCommand + " -C \"" + fs::absolute(outputDir).string() + "\"";

        // 添加统计选项时使用正确转义
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

        // 准备构建统计
        omake::BuildStats stats;
        size_t totalFiles = config.fileRules.size();

        // 执行命令并捕获输出
        if (showStats) {
            stats.start();

            // 使用 popen 捕获输出
            FILE* pipe = popen(command.c_str(), "r");
            if (!pipe) {
                throw std::runtime_error("无法执行命令: " + command);
            }

            // 用于线程同步的原子标志
            std::atomic<bool> keepRunning(true);

            // 启动线程用于显示进度条
            std::thread progressThread([&]() {
                while (keepRunning) {
                    // 获取当前输出（逐行读取）
                    std::string output = readBuffer(pipe);
                    if (!output.empty()) {
                        // 解析进度
                        auto [builtFiles, outputTotal] = omake::Builder::getBuildStats(output);
                        if (outputTotal == 0) outputTotal = totalFiles;

                        // 更新统计
                        stats.update(builtFiles, outputTotal);

                        // 显示进度条和统计信息（增加强制刷新）
                        std::cout << "\r" << stats.getProgressBar() << " " << stats.getSummary() << std::flush;
                    }

                    // 调整睡眠时间以平衡响应速度
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            });

            std::cout << "\r" << stats.getProgressBar() << " " << stats.getSummary() << std::flush;
            // 等待进程完成
            int result = pclose(pipe);
            keepRunning = false;
            progressThread.join();
            
            // 强制清除进度行
            std::cout << "\r\033[K";  // ANSI转义序列清空行

            // 输出最终结果
            std::cout << "\n构建" << (result == 0 ? "成功" : "失败")
                << "，共 " << totalFiles << " 个文件\n";

            if (result != 0) {
                std::cerr << "构建失败，退出代码: " << result << std::endl;
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