//
// Created by 李卓 on 24-6-12.
//

#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include "INodeCache.h"
#include "VDisk.h"

/*-------------------文件描述符-------------------*/
// 最大文件描述符数目
#define MAX_FD_NUM 128
// 文件打开模式，采用二进制描述，在模式控制时使用位运算
#define MODE_R 0b001
#define MODE_W 0b010
// 是否为追加模式
#define MODE_A 0b100
// 默认文件打开模式
#define MODE_DEFAULT MODE_R

/**
 * 文件描述句柄
 * 当前文件偏移量（调用read()和write()时更新，或使用lseek()直接修改）
 * 对应的INode引用
 * 文件打开模式 r/w/rw
 */
struct FileDescriptor
{
    // 文件名
    std::string fileName;
    // 当前文件偏移量
    unsigned int offset;
    // INode引用
    INode* iNode;
    // 文件打开模式
    int mode;
};

/**
 * 文件管理类
 * 用于管理文件系统中的文件文件描述符和文件缓存的对应关系
 */
class FileManager
{
public:
    // 文件描述符的位图，用于查找空闲的文件描述符
    std::bitset<MAX_FD_NUM> fdBitmap;
    // 文件描述符表，用于查找文件描述符对应的文件句柄
    std::map<int, FileDescriptor> fdTable;
    // 文件缓存，用于缓存文件内容
    INodeCache* iNodeCache;
    FileManager();
    int getFreeFd();
    bool freeFd(int fd);
    bool setFdItem(const int fd, FileDescriptor fdItem);
    bool setFdOffset(int fd, int offset);
    int getFdOffset(int fd);
    int doOpenFile(INode& iNode, const std::string& fileName, int mode, bool& clear);
    bool doCloseFile(int fd);
    FileDescriptor* getFdItem(int fd);
};

#endif //FILEMANAGER_H
