//
// Created by 李卓 on 24-6-12.
//

#include "FileManager.h"

#include <utility>

/**
 * FileManager类的构造函数
 */
FileManager::FileManager()
{
    // 初始化文件描述符位图
    this->fdBitmap.reset();
    // 初始化文件描述符表
    this->fdTable.clear();
    // 初始化文件缓存
    this->iNodeCache = new INodeCache(MAX_INODE_NUM);
}

/**
 * 获取一个空闲的文件描述符
 */
int FileManager::getFreeFd()
{
    // 查找空闲的文件描述符，即查找位图中第一个为0的位
    int fd = -1;
    for (int i = 0; i < MAX_FD_NUM; i++)
    {
        if (!this->fdBitmap.test(i))
        {
            fd = i;
            break;
        }
    }
    // 如果找到了
    if (fd < MAX_FD_NUM && fd >= 0)
    {
        // 设置该文件描述符为已使用
        this->fdBitmap.set(fd);
    }
    return fd;
}

/**
 * 释放一个文件描述符
 * @param fd 文件描述符
 */
bool FileManager::freeFd(int fd)
{
    // 如果文件描述符不合法
    if (fd < 0 || fd >= MAX_FD_NUM)
    {
        return false;
    }
    // 设置该文件描述符为空闲
    this->fdBitmap.reset(fd);
    return true;
}

/**
 * 设置文件描述符对应的文件句柄
 * @param fd 文件描述符
 * @param fdItem 文件句柄
 * @return
 */
bool FileManager::setFdItem(const int fd, FileDescriptor fdItem)
{
    // 如果文件描述符不合法
    if (fd < 0 || fd >= MAX_FD_NUM)
    {
        return false;
    }
    // 设置文件描述符对应的文件句柄
    this->fdTable[fd] = std::move(fdItem);
    return true;
}

/**
 * 设置文件描述符对应文件句柄的文件偏移量
 * @param fd 文件描述符
 * @param offset 文件偏移量
 */
bool FileManager::setFdOffset(const int fd, int offset)
{
    // 如果文件描述符不合法
    if (fd < 0 || fd >= MAX_FD_NUM)
    {
        return false;
    }
    // 设置文件描述符对应文件句柄的文件偏移量
    this->fdTable[fd].offset = offset;
    return true;
}

/**
 * 读取文件描述符对应文件句柄的文件偏移量
 * @param fd 文件描述符
 * @return 文件偏移量
 */
int FileManager::getFdOffset(const int fd)
{
    // 如果文件描述符不合法
    if (fd < 0 || fd >= MAX_FD_NUM)
    {
        return -1;
    }
    // 读取文件描述符对应文件句柄的文件偏移量
    return this->fdTable[fd].offset;
}

/**
 * 打开文件的文件描述符相关，建立inode、文件描述符、文件缓存的对应关系
 * @param INode inode
 * @param fileName 文件名
 * @param mode 文件打开模式
 * @return 文件描述符
 */
int FileManager::doOpenFile(INode &iNode, const std::string& fileName, int mode, bool &clear)
{
    // 首先获取一个空闲的文件描述符
    int fd = getFreeFd();
    // 如果文件描述符不合法
    if (fd < 0)
    {
        return -1;
    }
    // 创建一个新的文件句柄
    FileDescriptor fdItem;
    // 设置文件名
    fdItem.fileName = fileName;
    // 设置模式
    fdItem.mode = mode;
    // 设置inode
    fdItem.iNode = &iNode;
    // 如果mode有追加，那么设置文件偏移量为文件大小
    if (mode & MODE_A)
    {
        fdItem.offset = iNode.iNodeSize;
    }
    else
    {
        fdItem.offset = 0;
    }
    // 根据模式设置文件偏移量
    if (mode & MODE_A)
    {
        // 追加模式
        fdItem.offset = this->fdTable[fd].iNode->iNodeSize;
    }
    else
    {
        // 读写模式
        fdItem.offset = 0;
        // 如果是写模式，清空文件内容
        if (mode & MODE_W)
        {
            // 留到后面清空
            clear = true;
        }
    }
    // 设置文件描述符对应的文件句柄
    setFdItem(fd, fdItem);
    // 返回文件描述符
    return fd;
}

/**
 * 关闭文件的文件描述符相关，释放文件描述符
 * @param fd 文件描述符
 * @return 是否成功
 */
bool FileManager::doCloseFile(int fd)
{
    // 如果文件描述符不合法
    if (fd < 0 || fd >= MAX_FD_NUM)
    {
        return false;
    }
    // 释放文件描述符
    freeFd(fd);
    // 删除文件句柄
    fdTable.erase(fd);
    return true;
}

/**
 * 读取指定文件描述符对应的文件句柄
 * @param fd 文件描述符
 * @return 文件句柄
 */
FileDescriptor* FileManager::getFdItem(int fd)
{
    // 如果文件描述符不合法
    if (fd < 0 || fd >= MAX_FD_NUM)
    {
        return nullptr;
    }
    // 如果文件描述符对应的文件句柄不存在
    if (fdTable.find(fd) == fdTable.end())
    {
        return nullptr;
    }
    // 读取文件描述符对应的文件句柄
    return &fdTable[fd];
}




