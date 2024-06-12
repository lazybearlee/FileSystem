//
// Created by 李卓 on 24-6-12.
//

#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include "VDisk.h"

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
    int offset;
    // INode引用
    INode* iNode;
    // INode地址
    int iNodeAddr;
    // 文件打开模式
    std::string mode;
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
    FileManager();
    int getFreeFd();
    bool freeFd(int fd);
};

#endif //FILEMANAGER_H
