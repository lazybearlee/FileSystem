#include <iostream>
#include <sstream>
#include "FileSystem.h"

int main() {
    std::cout << "Welcome to the file system!" << std::endl;
    // 打印文件系统的版本、作者等信息
    std::cout << "Version: " << VERSION << std::endl;
    std::cout << "Author: " << AUTHOR << std::endl;
    std::cout << "Email: " << EMAIL << std::endl;
    std::cout << std::endl;

    FileSystem fs;
    std::string input;

    while (true) {
        // 如果已经载入文件系统，那么显示当前路径
        if (fs.installed) {
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
        else {
            fs.executeInFS(input);
        }
    }
    return 0;
}
