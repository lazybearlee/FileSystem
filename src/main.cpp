#include <iostream>
#include <sstream>
#include "FileSystem.h"

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

    // testIO();

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

        if (command == "exit") {
            fs.save();
            fs.uninstall();
        }
        // 如果为"quit"命令，退出程序
        else if (command == "quit") {
            // 析构函数
            fs.~FileSystem();
            exit(0);
        }
        else if (command == "new") {
            std::string systemName;
            iss >> systemName;
            if (systemName.empty()) {
                std::cerr << "Error: Missing system name for 'new' command." << std::endl;
                continue;
            }
            fs.create(systemName);
        }
        else if (command == "sys") {
            std::string filename;
            iss >> filename;
            if (filename.empty()) {
                std::cerr << "Error: Missing filename for 'sys' command." << std::endl;
                continue;
            }
            fs.load(filename);
        }
        else if (command == "super")
        {
            fs.printSuperBlock();
        }
        else if (command == "info")
        {
            fs.printBuffer();
            fs.readSuperBlock();
            fs.printSystemInfo();
        }
        else {
            fs.executeInFS(input);
        }
    }
    return 0;
}
