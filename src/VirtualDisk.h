//
// Created by 李卓 on 24-6-9.
//

#ifndef VIRTUALDISK_H
#define VIRTUALDISK_H

#include <string>
#include <vector>
#include <bitset>
#include <cstdint>

// Constants
const size_t BLOCK_SIZE = 4096;  // 4KB
const size_t INODE_SIZE = 128;   // 128B
const size_t TOTAL_BLOCKS = 1024;  // Total number of blocks
const size_t TOTAL_INODES = 128;   // Total number of inodes

// Superblock structure
struct SuperBlock {
    uint32_t totalInodes;
    uint32_t totalBlocks;
    uint32_t blocksPerGroup;
    uint32_t inodesPerGroup;
    uint32_t firstDataBlock;
};

// Block group descriptor structure
struct BlockGroupDescriptor {
    uint32_t blockBitmap;
    uint32_t inodeBitmap;
    uint32_t inodeTable;
    uint32_t freeBlocksCount;
    uint32_t freeInodesCount;
};

// Inode structure
struct Inode {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t linksCount;
    uint32_t blocks;
    uint32_t flags;
    uint32_t block[15];
};

// VirtualDisk class
class VirtualDisk {
public:
    VirtualDisk();
    void format();
    void loadFromFile(const std::string &filename);
    void saveToFile(const std::string &filename);

    // Read and write functions
    void readBlock(uint32_t blockNum, char* buffer);
    void writeBlock(uint32_t blockNum, const char* buffer);

private:
    SuperBlock superBlock;
    std::vector<BlockGroupDescriptor> blockGroupDescriptors;
    std::bitset<TOTAL_BLOCKS> blockBitmap;
    std::bitset<TOTAL_INODES> inodeBitmap;
    std::vector<Inode> inodes;
    std::vector<char> dataBlocks;

    void initialize();
};

#endif // VIRTUALDISK_H

