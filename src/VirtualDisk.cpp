//
// Created by 李卓 on 24-6-9.
//
#include "VirtualDisk.h"
#include <fstream>
#include <iostream>
#include <cstring>

// Constructor
VirtualDisk::VirtualDisk() {
    initialize();
}

// Initialize the virtual disk
void VirtualDisk::initialize() {
    superBlock = {TOTAL_INODES, TOTAL_BLOCKS, TOTAL_BLOCKS, TOTAL_INODES, 1};
    blockGroupDescriptors = { {2, 3, 4, TOTAL_BLOCKS - 1, TOTAL_INODES - 1} };
    blockBitmap.reset();
    inodeBitmap.reset();
    inodes.resize(TOTAL_INODES);
    dataBlocks.resize(TOTAL_BLOCKS * BLOCK_SIZE);
}

// Format the virtual disk
void VirtualDisk::format() {
    initialize();
}

// Load the virtual disk from a file
void VirtualDisk::loadFromFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << " for reading." << std::endl;
        return;
    }

    file.read(reinterpret_cast<char*>(&superBlock), sizeof(superBlock));
    file.read(reinterpret_cast<char*>(blockGroupDescriptors.data()), blockGroupDescriptors.size() * sizeof(BlockGroupDescriptor));
    file.read(reinterpret_cast<char*>(&blockBitmap), sizeof(blockBitmap));
    file.read(reinterpret_cast<char*>(&inodeBitmap), sizeof(inodeBitmap));
    file.read(reinterpret_cast<char*>(inodes.data()), inodes.size() * sizeof(Inode));
    file.read(dataBlocks.data(), dataBlocks.size());
    file.close();
}

// Save the virtual disk to a file
void VirtualDisk::saveToFile(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&superBlock), sizeof(superBlock));
    file.write(reinterpret_cast<const char*>(blockGroupDescriptors.data()), blockGroupDescriptors.size() * sizeof(BlockGroupDescriptor));
    file.write(reinterpret_cast<const char*>(&blockBitmap), sizeof(blockBitmap));
    file.write(reinterpret_cast<const char*>(&inodeBitmap), sizeof(inodeBitmap));
    file.write(reinterpret_cast<const char*>(inodes.data()), inodes.size() * sizeof(Inode));
    file.write(dataBlocks.data(), dataBlocks.size());
    file.close();
}

// Read a block from the virtual disk
void VirtualDisk::readBlock(uint32_t blockNum, char* buffer) {
    if (blockNum >= TOTAL_BLOCKS) {
        std::cerr << "Error: Block number out of range." << std::endl;
        return;
    }
    std::memcpy(buffer, dataBlocks.data() + blockNum * BLOCK_SIZE, BLOCK_SIZE);
}

// Write a block to the virtual disk
void VirtualDisk::writeBlock(uint32_t blockNum, const char* buffer) {
    if (blockNum >= TOTAL_BLOCKS) {
        std::cerr << "Error: Block number out of range." << std::endl;
        return;
    }
    std::memcpy(dataBlocks.data() + blockNum * BLOCK_SIZE, buffer, BLOCK_SIZE);
}
