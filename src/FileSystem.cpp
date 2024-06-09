//
// Created by 李卓 on 24-6-9.
//
#include "FileSystem.h"
#include <iostream>
#include <cstring>

/**
 * 构造函数
 */
FileSystem::FileSystem()
{
    nextGroupID = 0;
    nextUserID = 0;
    superBlock = {};
    userState = {};
    userState.isLogin = false;
    // 设置char[] userState.userName 为"root"
    strcpy(userState.userName, "root");
    // 设置char[] userState.userGroup 为"root"
    strcpy(userState.userGroup, "root");
}

/**
 * 获取主机名
 */
void FileSystem::getHostName()
{
    // 获取当前主机名
    memset(hostName, 0, sizeof(hostName));
    // 如果是windows系统，采用gethostname函数
#ifdef _WIN32
    DWORD size = sizeof(hostName);
    GetComputerName(hostName, &size);
#else
    // 如果是linux系统
    gethostname(hostName, sizeof(hostName));
#endif
}

/**
 * 分配一个inode
 * @brief 在inode位图中找到一个空闲的inode，分配给用户并更新inode位图
 * @return 分配的inode地址
 */
int FileSystem::allocateINode()
{
    // 如果没有空闲inode
    if (superBlock.freeINodeNum == 0)
    {
        std::cerr << "Error: allocateINode---No free inode." << std::endl;
        return -1;
    }
    // 顺序查找inode位图，找到第一个空
    for (int i = 0; i < superBlock.iNodeNum; i++)
    {
        if (!iNodeBitmap.test(i))
        {
            // 更新inode位图
            iNodeBitmap[i] = true;
            // 更新超级块
            superBlock.freeINodeNum--;
            fw.seekp(superBlockStartPos, std::ios::beg);
            fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
            // 更新inode位图，仅更新一个位
            fw.seekp(iNodeBitmapStartPos + i, std::ios::beg);
            // 用其他变量拿到iNodeBitmap[i]的地址，然后写入iNodeBitmap[i]的值
            bool temp = iNodeBitmap[i];
            // 写回
            fw.write(reinterpret_cast<char*>(&temp), sizeof(temp));
            // 清空缓冲区
            fw.flush();
            // 返回inode地址
            return iNodeStartPos + i * superBlock.iNodeSize;
        }
    }
    return -1;
}

/**
 * 释放一个inode
 * @param iNodeAddr inode地址
 */
bool FileSystem::freeINode(int iNodeAddr)
{
    // 首先判断inode地址是否合法
    if (iNodeAddr < iNodeStartPos || iNodeAddr >= iNodeStartPos + superBlock.iNodeNum * superBlock.iNodeSize)
    {
        std::cerr << "Error: freeINode---Invalid inode address." << std::endl;
        return false;
    }
    // 计算inode在inode位图中的位置
    unsigned short iNodeIndex = (iNodeAddr - iNodeStartPos) / superBlock.iNodeSize;
    // 清空inode
    INode iNode = {};
    fw.seekp(iNodeAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
    // 更新inode位图
    iNodeBitmap[iNodeIndex] = false;
    // 更新超级块
    superBlock.freeINodeNum++;
    fw.seekp(superBlockStartPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    // 更新inode位图
    fw.seekp(iNodeBitmapStartPos + iNodeIndex, std::ios::beg);
    bool temp = iNodeBitmap[iNodeIndex];
    fw.write(reinterpret_cast<char*>(&temp), sizeof(temp));
    // 清空缓冲区
    fw.flush();
    return true;
}

/**
 * 分配一个数据块
 * @return 分配的数据块地址
 */
int FileSystem::allocateBlock()
{
    // 从空闲块堆栈中分配一个块
    if (superBlock.freeBlockNum == 0)
    {
        std::cerr << "Error: allocateBlock---No free block." << std::endl;
        return -1;
    }
    // 计算当前栈对应组的栈顶
    int ret = -1;
    unsigned int groupTop = (superBlock.freeBlockNum - 1) / superBlock.blockGroupSize;
    // 如果当前已经是栈底，那么就要将栈底返回的同时，把栈底的下一组的栈顶更新到当前组的栈顶
    if (groupTop == 0)
    {
        ret = superBlock.freeBlockAddr;
        // 更新栈顶
        superBlock.freeBlockAddr = superBlock.freeBlockStack[0];
        // 更新超级块
        superBlock.freeBlockNum--;
        fw.seekp(superBlock.freeBlockAddr, std::ios::beg);
        fw.write(reinterpret_cast<char*>(&superBlock.freeBlockStack), sizeof(superBlock.freeBlockStack));
    }
    else
    {
        // 返回栈顶
        ret = superBlock.freeBlockStack[groupTop];
        superBlock.freeBlockStack[groupTop] = -1;
        // 更新栈顶
        superBlock.freeBlockNum--;
        // 更新超级块
        fw.seekp(superBlock.freeBlockAddr, std::ios::beg);
        fw.write(reinterpret_cast<char*>(&superBlock.freeBlockAddr), sizeof(superBlock.freeBlockAddr));
    }
    // 更新超级块
    fw.seekp(superBlockStartPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    // 清空缓冲区
    fw.flush();

    // 更新数据块位图
    blockBitmap[(ret - blockStartPos) / BLOCK_SIZE] = true;
    fw.seekp(blockBitmapStartPos + (ret - blockStartPos) / BLOCK_SIZE, std::ios::beg);
    bool temp = blockBitmap[(ret - blockStartPos) / BLOCK_SIZE];
    fw.write(reinterpret_cast<char*>(&temp), sizeof(temp));
    // 清空缓冲区
    fw.flush();

    return ret;
}

/**
 * 释放一个数据块
 * @param blockAddr 数据块地址
 */
bool FileSystem::freeBlock(int blockAddr)
{
    // 如果没有非空闲数据块
    if (superBlock.freeBlockNum == superBlock.blockNum)
    {
        std::cerr << "Error: freeBlock---No nofree block." << std::endl;
        return false;
    }
    // 首先判断数据块地址是否合法
    if (blockAddr < blockStartPos || blockAddr >= blockStartPos + superBlock.blockNum * superBlock.blockSize || (blockAddr - blockStartPos) % BLOCK_SIZE != 0)
    {
        std::cerr << "Error: freeBlock---Invalid block address." << std::endl;
        return false;
    }

    // 计算数据块在数据块位图中的位置
    unsigned int blockIndex = (blockAddr - superBlock.blockStartPos) / superBlock.blockSize;
    // 如果数据块已经是空闲的
    if (!blockBitmap[blockIndex])
    {
        std::cerr << "Error: freeBlock---Block is already free." << std::endl;
        return false;
    }
    // 计算当前栈对应组的栈顶
    unsigned int groupTop = (superBlock.freeBlockNum - 1) / superBlock.blockGroupSize;
    // 清空数据块
    char emptyBlock[BLOCK_SIZE] = {};
    fw.seekp(blockAddr, std::ios::beg);
    fw.write(emptyBlock, sizeof(emptyBlock));

    // 分两种情况，如果当前栈已满，那么就要将当前栈的栈顶更新到下一组的栈顶；否则直接将栈顶更新到当前栈
    if (groupTop == superBlock.blockGroupSize - 1)
    {
        superBlock.freeBlockStack[0] = superBlock.freeBlockAddr;
        // 将栈中其他元素清空
        for (int i = 1; i < superBlock.blockGroupSize; i++)
        {
            superBlock.freeBlockStack[i] = -1;
        }
        // 更新栈顶
        fw.seekp(blockAddr, std::ios::beg);
        fw.write(reinterpret_cast<char*>(&superBlock.freeBlockStack), sizeof(superBlock.freeBlockStack));
    }
    else
    {
        // 在栈顶上放上这个被释放的块
        superBlock.freeBlockStack[groupTop + 1] = blockAddr;
    }

    // 更新超级块
    superBlock.freeBlockNum++;
    fw.seekp(superBlockStartPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));

    // 更新位图
    blockBitmap[blockIndex] = false;
    fw.seekp(blockBitmapStartPos + blockIndex, std::ios::beg);
    bool temp = blockBitmap[blockIndex];
    fw.write(reinterpret_cast<char*>(&temp), sizeof(temp));
    // 清空缓冲区
    fw.flush();

    return true;
}

/**
 * 格式化文件系统
 * @return 是否格式化文件系统成功
 */
bool FileSystem::formatFileSystem()
{
    /*-------------------初始化超级块-------------------*/
    // 设置超级块大小
    superBlock.superBlockSize = sizeof(SuperBlock);
    // 设置inode相关
    superBlock.iNodeSize = sizeof(INode);
    superBlock.iNodeNum = MAX_INODE_NUM;
    superBlock.freeINodeNum = MAX_INODE_NUM;
    // 设置数据块相关
    superBlock.blockSize = BLOCK_SIZE;
    superBlock.blockNum = BLOCK_NUM;
    superBlock.freeBlockNum = BLOCK_NUM;
    // 设置inode位图和数据块位图的起始位置
    superBlock.iNodeBitmapPos = iNodeBitmapStartPos;
    superBlock.blockBitmapPos = blockBitmapStartPos;
    // 设置inode节点的起始位置
    superBlock.iNodeStartPos = iNodeStartPos;
    // 设置数据块的起始位置
    superBlock.blockStartPos = blockStartPos;
    // 设置每个块组的块数
    superBlock.blockGroupSize = FREE_BLOCK_STACK_SIZE;
    // 设置空闲块堆栈栈顶为第一块BLOCK
    superBlock.freeBlockAddr = blockStartPos;
    // 设置超级块位置
    superBlock.superBlockPos = superBlockStartPos;

    /*-------------------初始化inode位图-------------------*/
    // 初始化inode位图
    iNodeBitmap.reset();
    // 初始化文件内容中的inode位图
    fw.seekp(iNodeBitmapStartPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&iNodeBitmap), sizeof(iNodeBitmap));

    /*-------------------初始化数据块位图-------------------*/
    // 初始化数据块位图
    blockBitmap.reset();
    // 初始化文件内容中的数据块位图
    fw.seekp(blockBitmapStartPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&blockBitmap), sizeof(blockBitmap));

    /*-------------------
     * 初始化Block，将空闲栈进行初始化
     * 初始化空闲块堆栈，从最后一个组开始将数据区按扇区分组，每组的第一个扇区存放下一个组的地址
     * 每个组大小为 32*512B=16KB
     *-------------------*/
    for (int i = BLOCK_NUM / BLOCK_GROUP_SIZE - 1; i >= 0; i--)
    {
        // 将每个组的第一个扇区存放下一个组的地址
        if (i == BLOCK_NUM / BLOCK_GROUP_SIZE - 1)
        {
            // 最后一个组的第一个扇区存放-1，表示没有下一个组
            superBlock.freeBlockStack[0] = -1;
        }
        else
        {
            // 其他组的第一个扇区存放下一个组的地址
            superBlock.freeBlockStack[0] = blockStartPos + (i + 1) * BLOCK_GROUP_SIZE * BLOCK_SIZE;
        }
        // 将每个组的其他扇区存放空闲块
        for (int j = 1; j < BLOCK_GROUP_SIZE; j++)
        {
            superBlock.freeBlockStack[j] = blockStartPos + i * BLOCK_GROUP_SIZE * BLOCK_SIZE + j * BLOCK_SIZE;
        }
        // 将组写入文件，首先定位到组的第一个扇区
        fw.seekp(blockStartPos + i * BLOCK_GROUP_SIZE * BLOCK_SIZE, std::ios::beg);
        fw.write(reinterpret_cast<char*>(&superBlock.freeBlockStack), sizeof(superBlock.freeBlockStack));
    }
    // 将超级块写入文件
    fw.seekp(superBlockStartPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));

    // 清空缓冲区
    fw.flush();

    // 读取位图
    fr.seekg(iNodeBitmapStartPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&iNodeBitmap), sizeof(iNodeBitmap));
    fr.seekg(blockBitmapStartPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&blockBitmap), sizeof(blockBitmap));

    /*-------------------初始化目录与INode-------------------*/
    // 初始化根目录inode
    INode rootINode;
    // 为根目录inode申请inode空间
    int iNodeAddr = allocateINode();
    if (iNodeAddr == -1)
    {
        std::cerr << "Error: formatFileSystem---Failed to allocate inode for root directory." << std::endl;
        return false;
    }
    // 为根目录inode申请数据块
    int blockAddr = allocateBlock();
    if (blockAddr == -1)
    {
        std::cerr << "Error: formatFileSystem---Failed to allocate block for root directory." << std::endl;
        return false;
    }

    // 声明一个目录项数组，用于存放根目录的目录项
    DirItem dirItem[16] = {};
    // 初始化第一个目录项为"."
    strcpy(dirItem[0].itemName, ".");
    dirItem[0].iNodeAddr = iNodeAddr;
    // 将目录写回
    fw.seekp(blockAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&dirItem), sizeof(dirItem));

    // 给根目录inode赋值
    // inode号
    rootINode.iNodeNum = 0;
    // 大小
    rootINode.iNodeSize = superBlock.blockSize;
    // 时间
    rootINode.iNodeCreateTime = time(nullptr);
    rootINode.iNodeModifyTime = time(nullptr);
    rootINode.iNodeAccessTime = time(nullptr);
    // 用户名与用户组
    strcpy(rootINode.iNodeOwner, userState.userName);
    strcpy(rootINode.iNodeGroup, userState.userGroup);
    // 链接，目前为1，即初始化时仅链接到上面的目录
    rootINode.iNodeLink = 1;
    // 设置inode数据项
    rootINode.iNodeBlockPointer[0] = blockAddr;
    for (int i = 1; i < 16; ++i)
    {
        rootINode.iNodeBlockPointer[i] = -1;
    }
    // 设置默认权限
    rootINode.iNodeMode = DIR_TYPE | DEFAULT_DIR_MODE;

    // 写回
    fw.seekp(iNodeAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&rootINode), sizeof(INode));

    // 清空缓冲区
    fw.flush();
    return true;
}

/**
 * 初始化文件系统的其他配置
 * @brief 要求用户自己
 */
void FileSystem::initSystemConfig()
{
}

/**
 * 创建文件系统
 * @param systemName 文件系统名
 */
void FileSystem::create(const std::string& systemName)
{
    // 重组文件名，加上后缀
    std::string filename = systemName + ".vdisk";
    // 尝试创建文件
    fw.open(filename, std::ios::out | std::ios::binary);
    if (!fw.is_open())
    {
        std::cerr << "Error: Failed to create virtual disk. " << filename << std::endl;
        return;
    }
    // 现在可以打开文件
    fr.open(filename, std::ios::in | std::ios::binary);

    // 获取主机名
    getHostName();
    // 设置根目录inode
    rootINodeAddr = iNodeStartPos;
    // 设置当前目录inode
    currentDir = rootINodeAddr;
    // 设置当前目录名
    strcpy(currentDirName, "/");

    // 格式化文件系统
    if (formatFileSystem())
    {
        std::cout << "File system created successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: Failed to create file system." << std::endl;
    }

    // 初始化文件系统的其他配置
    initSystemConfig();

    // 安装文件系统
    if (installFileSystem())
    {
        std::cout << "File system is installed after created successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: Failed to load file system." << std::endl;
    }
}


/**
 * 安装文件系统
 * @brief 将虚拟磁盘文件中的信息加载到内存中
 * @return 是否安装文件系统成功
 */
bool FileSystem::installFileSystem()
{
    // 读取超级块
    fr.seekg(superBlockStartPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    if (fr.fail())
    {
        std::cerr << "Error: Failed to read super block." << std::endl;
        return false;
    }
    // 读取inode位图
    fr.seekg(iNodeBitmapStartPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&iNodeBitmap), sizeof(iNodeBitmap));
    if (fr.fail())
    {
        std::cerr << "Error: Failed to read inode bitmap." << std::endl;
        return false;
    }
    // 读取数据块位图
    fr.seekg(blockBitmapStartPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&blockBitmap), sizeof(blockBitmap));
    if (fr.fail())
    {
        std::cerr << "Error: Failed to read block bitmap." << std::endl;
        return false;
    }
    return true;
}

/**
 * 打开虚拟磁盘文件
 */
void FileSystem::load(const std::string& filename)
{
    // 首先只读打开文件
    fr.open(filename, std::ios::in | std::ios::binary);
    if (!fr.is_open())
    {
        std::cerr << "Error: Failed to load virtual disk. " << filename << std::endl;
        return;
    }
    // 只写打开文件
    fw.open(filename, std::ios::out | std::ios::binary);
    if (!fw.is_open())
    {
        std::cerr << "Error: Failed to load virtual disk. " << filename << std::endl;
        return;
    }

    // 获取主机名
    getHostName();
    // 设置根目录inode
    rootINodeAddr = iNodeStartPos;
    // 设置当前目录inode
    currentDir = rootINodeAddr;
    // 设置当前目录名
    strcpy(currentDirName, "/");

    // 安装文件系统
    if (installFileSystem())
    {
        std::cout << "File system loaded successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: Failed to load file system." << std::endl;
    }
}

/**
 * 保存文件系统
 */
void FileSystem::save()
{
    std::cout << "Saving file system..." << std::endl;
    // 保存超级块
    // fw.seekp(superBlockStartPos, std::ios::beg);
    // fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    // // 保存inode位图
    // fw.seekp(iNodeBitmapStartPos, std::ios::beg);
    // fw.write(reinterpret_cast<char*>(&iNodeBitmap), sizeof(iNodeBitmap));
    // // 保存数据块位图
    // fw.seekp(blockBitmapStartPos, std::ios::beg);
    // fw.write(reinterpret_cast<char*>(&blockBitmap), sizeof(blockBitmap));
    // // 清空缓冲区
    fw.flush();
    fr.close();
    fw.close();
    std::cout << "File system saved successfully." << std::endl;
}


/**
 *
 * @param command
 */
void FileSystem::execute(const std::string& command)
{
    std::cout << "Executing command: " << command << std::endl;
    // Implement command execution
}

/**
 * 打印superBlock信息
 */
void FileSystem::printSuperBlock()
{
    std::cout << "SuperBlock:" << std::endl;
    std::cout << "\tSize: " << superBlock.superBlockSize << std::endl;
    // inode相关信息
    std::cout << "Inode:" << std::endl;
    std::cout << "\tSize: " << superBlock.iNodeSize << std::endl;
    std::cout << "\tNumber: " << superBlock.iNodeNum << std::endl;
    std::cout << "\tFree: " << superBlock.freeINodeNum << std::endl;
    std::cout << "\tStart position: " << iNodeStartPos << std::endl;
    // 数据块相关信息
    std::cout << "Block:" << std::endl;
    std::cout << "\tSize: " << superBlock.blockSize << std::endl;
    std::cout << "\tNumber: " << superBlock.blockNum << std::endl;
    std::cout << "\tFree: " << superBlock.freeBlockNum << std::endl;
    std::cout << "\tStart position: " << blockStartPos << std::endl;
    // 位图
    std::cout << "Bitmap:" << std::endl;
    // inode位图,顺带打印inode位图
    std::cout << "\tInode bitmap start position: " << iNodeBitmapStartPos << std::endl;
    std::cout << "\tInode bitmap: ";
    for (int i = 0; i < superBlock.iNodeNum; i++)
    {
        std::cout << iNodeBitmap[i];
    }
    std::cout << std::endl;
    // 数据块位图
    std::cout << "\tBlock bitmap start position: " << blockBitmapStartPos << std::endl;
    std::cout << "\tBlock bitmap: ";
    for (int i = 0; i < superBlock.blockNum; i++)
    {
        std::cout << blockBitmap[i];
    }
    std::cout << std::endl;
}
