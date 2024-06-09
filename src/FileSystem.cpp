//
// Created by 李卓 on 24-6-9.
//
#include "FileSystem.h"
#include <iostream>

void FileSystem::create(const std::string &systemName) {
    std::cout << "Creating new file system: " << systemName << "..." << std::endl;
    disk.format();
    currentFilename = systemName + ".sys";
    std::cout << "Saving file system to " << currentFilename << "..." << std::endl;
    disk.saveToFile(currentFilename);
}

void FileSystem::load(const std::string &filename) {
    std::cout << "Loading file system from " << filename << "..." << std::endl;
    disk.loadFromFile(filename);
    currentFilename = filename;
}

void FileSystem::save() {
    if (currentFilename.empty()) {
        std::cerr << "Error: No file system to save." << std::endl;
        return;
    }
    std::cout << "Saving file system to " << currentFilename << "..." << std::endl;
    disk.saveToFile(currentFilename);
}

void FileSystem::execute(const std::string &command) {
    std::cout << "Executing command: " << command << std::endl;
    // Implement command execution
}
