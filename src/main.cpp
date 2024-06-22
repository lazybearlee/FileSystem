#include <iostream>
#include <sstream>
#include "FileSystem.h"
#include "FSHelper.h"

/**
 * 打印系统欢迎信息
 */
void printSystemWelcome()
{
    // 一个大大大logo
    std::string fsLogo = R"(
  _______  _______
 |  _____||  _____|
 | |_____ | |_____
 |  _____||_____  |
 | |       _____| |
 |_|      |_______|)";

    std::string welcomeMessage = R"(
*******************************************
*                                         *
*           Welcome to the LiFS           *
*           File System!                  *
*                                         *
*******************************************)";

    std::cout << fsLogo << std::endl;
    std::cout << welcomeMessage << std::endl;
    // 打印文件系统的版本、作者等信息
    std::cout << "Version: " << VERSION << std::endl;
    std::cout << "Author: " << AUTHOR << std::endl;
    std::cout << "Email: " << EMAIL << std::endl;
    std::cout << std::endl;
}

int main() {
    // 打印系统信息
    printSystemWelcome();
    FileSystem fs;
    std::string input;

    while (true) {
        // 如果已经载入文件系统，那么显示当前路径
        if (fs.installed) {
            // 打印用户、主机名
            fs.printUserAndHostName();
            // 打印当前路径
            fs.printCurrentDir();
            std::cout << " $ ";
        }
        // 否则显示普通提示符
        else {
            std::cout << "> ";
        }
        std::getline(std::cin, input);
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        // 清楚当前cout中的内容，避免输出额外的空格
        std::cout.clear();

        if (command == "exit") {
            // 如果文件系统还未载入
            if (!fs.installed)
            {
                std::cerr << "Error: File system not installed." << std::endl;
                continue;
            }
            fs.save();
            fs.uninstall();
        }
        // 如果为"quit"命令，退出程序
        else if (command == "quit") {
            // 析构函数
            fs.~FileSystem();
            exit(0);
        }
        // 如果为clear命令，清屏
        else if (command == "clear")
        {
            fs.clearScreen();
        }
        else if (command == "new") {
            std::string systemName;
            iss >> systemName;
            if (systemName.empty())
            {
                // 如果没有输入文件系统名称,采用默认名称
                systemName = "LiFS";
            }
            fs.createFileSystem(systemName);
        }
        else if (command == "sfs") {
            // 如果已经载入文件系统，那么提示
            if (fs.installed)
            {
                std::cerr << "Error: File system already installed." << std::endl;
                continue;
            }
            std::string filename;
            iss >> filename;
            if (filename.empty())
            {
                // 如果没有输入文件名，采用默认文件名
                filename = "LiFS.vdisk";
            }
            fs.load(filename);
        }
        // help
        else if (command == "help")
        {
            std::string subCommand;
            iss >> subCommand;
            Helper::showHelpInformation("help");
        }
        else if (fs.installed)
        {
            fs.executeInFS(input);
        }
        else
        {
            std::cerr << "Error: File system not installed." << std::endl;
        }
    }
    return 0;
}
