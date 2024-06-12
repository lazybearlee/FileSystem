//
// Created by 李卓 on 24-6-12.
//

#include "FileManager.h"

/**
 * FileManager类的构造函数
 */
FileManager::FileManager()
{
    // 初始化文件描述符位图
    this->fdBitmap.reset();
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

