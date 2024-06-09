//
// Created by 李卓 on 24-6-9.
//
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>

/* 定义常量 */
#define MAX_BLOCK_SIZE 512
#define INODE_SIZE 128
#define MAX_DIRITEM_NAME_LEN 28

#define MAX_BLOCK_NUM 4294967295
#define MAX_INODE_NUM 65535
#define FREE_BLOCK_STACK_SIZE 64

/* 文件/目录属性定义 */
// 文件类型
#define FILE_TYPE 00000
// 目录类型
#define DIR_TYPE 01000
// 本用户读权限
#define OWNER_R	4<<6
//本用户写权限
#define OWNER_W	2<<6
// 本用户执行权限
#define OWNER_X	1<<6
// 组用户读权限
#define GROUP_R	4<<3
// 组用户写权限
#define GROUP_W	2<<3
// 组用户执行权限
#define GROUP_X	1<<3
// 其他用户读权限
#define OTHERS_R	4
// 其他用户写权限
#define OTHERS_W	2
// 其他用户执行权限
#define OTHERS_X	1
// 默认权限：文件，即本用户和组用户有读写权限，其他用户有读权限
#define DEFAULT_FILE_MODE 0664
// 默认权限：目录，即本用户有读写执行权限，其他用户和组用户有读和执行权限
#define DEFAULT_DIR_MODE 0755

/* 数据结构定义 */

/**
 * 超级块
 */
struct SuperBlock {
    /* 节点数目 */
    // inode节点数目，最多65535个
    unsigned short iNodeNum;
    // 数据块数目，最多4294967295个
    unsigned int blockNum;
    // 空闲inode节点数目
    unsigned short freeINodeNum;
    // 空闲数据块数目
    unsigned int freeBlockNum;

    /* 空闲块堆栈 */
    // 空闲块堆栈
    int freeBlockStack[FREE_BLOCK_STACK_SIZE];
    // 堆栈顶指针
    int stackTop;

    /* 大小 */
    // 磁盘块大小
    unsigned short blockSize;
    // inode节点大小
    unsigned short iNodeSize;
    // 超级块大小
    unsigned short superBlockSize;
    // 每个块组的块数
    unsigned short blockGroupSize;

    /* 磁盘分布（各个区块在虚拟磁盘中的位置） */
    // 超级块位置
    int superBlockPos;
    // inode位图位置
    int iNodeBitmapPos;
    // 数据块位图位置
    int blockBitmapPos;
    // inode节点起始位置
    int iNodeStartPos;
    // 数据块起始位置
    int blockStartPos;
};

/**
 * INODE节点
 */
struct INode
{
    // inode节点号
    unsigned short iNodeNum;
    // 文件类型与存取权限 采用八进制表示 例如：0755表示文件类型为普通文件，所有者有读写执行权限，组用户和其他用户有读和执行权限
    unsigned short iNodeMode;
    // 链接数
    unsigned short iNodeLink;
    // 文件所有者，字符串
    char iNodeOwner[10];
    // 文件所属组，字符串
    char iNodeGroup[10];
    // 文件大小
    unsigned int iNodeSize;
    // 文件创建时间，时间戳
    time_t iNodeCreateTime;
    // 文件修改时间，时间戳
    time_t iNodeModifyTime;
    // 文件访问时间，时间戳
    time_t iNodeAccessTime;
    // 文件数据块指针, 13个直接指针，1个一级间接指针，1个二级间接指针，1个三级间接指针
    unsigned int iNodeBlockPointer[16];
};

/**
 * 目录项
 */
struct DirItem
{
    // 目录项名
    char itemName[MAX_DIRITEM_NAME_LEN];
    // inode节点号
    unsigned short iNodeNum;
};


class FileSystem {
public:
    void create(const std::string &systemName);
    void load(const std::string &filename);
    void save();
    void execute(const std::string &command);
private:
    // 虚拟磁盘读

};

#endif // FILESYSTEM_H

