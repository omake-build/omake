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
            throw std::runtime_error("�޷��򿪹����ļ�: " + filename);
        }

        std::string line;
        int lineNum = 0;
        // ֧�ֵ����Ż�˫���ţ�֧��1��2������
        std::regex pattern(R"((\w+)\s+(?:\"([^\"]+)\"|'([^']+)')(?:\s*,\s*(?:\"([^\"]+)\"|'([^']+)'))?\s*)");

        while (std::getline(file, line)) {
            lineNum++;
            // �������к�ע�� (�������׿ո�)
            if (line.empty() || line.find_first_not_of(" \t") == std::string::npos ||
                line[line.find_first_not_of(" \t")] == '#') continue;

            std::smatch matches;
            if (std::regex_search(line, matches, pattern)) {
                // ��ʽת��Ϊ std::string
                std::string cmd = matches[1].str();

                try {
                    if (cmd == "filebuild") {
                        // �����ƥ����Ч�ԣ���2����3�ǵ�һ��������
                        std::string source = matches[2].matched ? matches[2].str() : matches[3].str();

                        // �ڶ���������������4����5
                        std::string header;
                        if (matches[4].matched) header = matches[4].str();
                        else if (matches[5].matched) header = matches[5].str();

                        config.fileRules.emplace_back(source, header);
                    }
                    else if (cmd == "buildfile") {
                        // ʹ����2����3��Ϊ����
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
                    // ע�⣺���ﲻ��Ҫ���� --starts������������ѡ��
                }
                catch (...) {
                    throw std::runtime_error("�������� [�� " + std::to_string(lineNum) + "]");
                }
            }
        }

        // ��֤Դ�ļ��б�
        if (config.fileRules.empty()) {
            throw std::runtime_error("û��ָ���κ�Դ�ļ�(filebuild)");
        }

        return config;
    }

} // namespace omake