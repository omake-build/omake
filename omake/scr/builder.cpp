#include "..\include\omake.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
namespace omake {

    void Builder::generateMakefile(const BuildConfig& config,
        const std::string& outputPath) {
        // ȷ�����Ŀ¼����
        fs::create_directories(outputPath);

        std::ofstream makefile(outputPath + "/Makefile");

        // 1. ����������
        makefile << "CC = " << config.compiler << "\n\n";

        // 2. ��̬����ѡ��
        makefile << "CFLAGS = ";
        #if defined(__linux__)
            makefile << "-static-libgcc -static-libstdc++ ";  // Linux静态链接
        #elif defined(__APPLE__)
            makefile << "-target x86_64-apple-darwin ";      // macOS目标平台
        #else
            makefile << "-mwindows ";                        // Windows隐藏控制台
        #endif
        for (const auto& opt : config.buildOptions) {
            makefile << opt << " ";
        }
        makefile << "\n\n";

        // 3. Դ�ļ��ռ���OBJS��
        makefile << "# Դ�ļ��б�\n";
        makefile << "SRCS = \\\n";
        for (const auto& rule : config.fileRules) {
            makefile << "    " << rule.first << " \\\n";
        }
        makefile << "\n\n";

        // 4. �����ļ��Զ��Ƶ� - ��Դ�ļ�����·������ȡ�ļ���
        makefile << "# �����ļ��б�\n";
        makefile << "OBJS = $(addsuffix .o, $(basename $(notdir $(SRCS))))\n\n";

        // 5. ��ƽ̨Ŀ��͹�������
        makefile << "# ��ƽ̨����\n";
        makefile << "ifeq ($(OS),Windows_NT)\n";
        makefile << "    TARGET := " << config.outputName << ".exe\n";
        makefile << "    RM := del /Q\n";
        makefile << "    SOURCE_DIR := " << fs::absolute(".").string() << "/\n"; // 统一使用正斜杠
        makefile << "else\n";
        makefile << "    TARGET := " << config.outputName << "\n";
        makefile << "    RM := rm -f\n";
        makefile << "    SOURCE_DIR := " << fs::absolute(".").string() << "/\n";
        makefile << "endif\n\n";

        // 6. ��Ҫ��������
        makefile << "all: $(TARGET)\n\n";
        makefile << "$(TARGET): $(OBJS)\n";
        makefile << "\t$(CC) $(CFLAGS) -o $@ $^\n\n";

        // 7. �ļ��������� - ʹ������·��ָ��Դ�ļ�
        for (const auto& rule : config.fileRules) {
            // ��ȡ�ļ���������·����
            fs::path sourcePath(rule.first);
            std::string fileName = sourcePath.filename().string();
            std::string objFile = fileName.substr(0, fileName.find_last_of('.')) + ".o";

            // �������� - ȷ��Դ�ļ�ʹ������·��
            makefile << objFile << ": $(SOURCE_DIR)" << rule.first << " ";

            // ����������ͷ�ļ������ָ����
            if (!rule.second.empty()) {
                makefile << "$(SOURCE_DIR)" << rule.second << " ";
            }

            makefile << "\n";
            makefile << "\t$(CC) $(CFLAGS) -c -o $@ $(SOURCE_DIR)" << rule.first << "\n\n";
        }

        // 8. ��������
        makefile << "clean:\n";
        makefile << "\t$(RM) $(TARGET) $(OBJS)\n\n";

        // 9. αĿ������
        makefile << ".PHONY: all clean\n";
    }

} // namespace omake