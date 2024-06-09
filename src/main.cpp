#include <iostream>
#include <sstream>
#include "FileSystem.h"

int main() {
    std::cout << "Welcome to the file system!" << std::endl;
    FileSystem fs;
    std::string input;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "exit") {
            fs.save();
        }
        if (command == "new") {
            std::string systemName;
            iss >> systemName;
            if (systemName.empty()) {
                std::cerr << "Error: Missing system name for 'new' command." << std::endl;
                continue;
            }
            fs.create(systemName);
        } else if (command == "sys") {
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
            fs.execute(input);
        }
    }
    return 0;
}
