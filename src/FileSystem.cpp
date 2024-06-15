//
// Created by 李卓 on 24-6-9.
//
#include "FileSystem.h"
#include <FSHelper.h>
#include <sstream>
#include <vector>

/**
 * 构造函数
 */
FileSystem::FileSystem(): dirINodeCache(MAX_INODE_CACHE_SIZE)
{
    installed = false;
    nextGroupID = 0;
    nextUserID = 0;
    superBlock = {};
    userState = {};
    userState.isLogin = false;
    // 设置char[] userState.userName 为"root"
    strcpy(userState.userName, "root");
    // 设置char[] userState.userGroup 为"root"
    strcpy(userState.userGroup, "root");
    userHomeDir = -1;
    fr = std::ifstream();
    fw = std::ofstream();
}

FileSystem::~FileSystem()
{
    // 关闭文件
    fr.close();
    fw.close();
    std::cout.flush();
    std::cout << "File closed." << std::endl;
    std::cout << "FileSystem is destroyed." << std::endl;
}

/**
 * 一个函数，用于打印fr的缓冲区前176个字节
 */
void FileSystem::printBuffer()
{
    char buffer[512] = {};
    // 调整指针位置
    fr.seekg(0, std::ios::beg);
    fr.read(buffer, sizeof(buffer));
    for (int i = 0; i < 512; i++)
    {
        std::cout << std::bitset<8>(buffer[i]) << " ";
    }
    std::cout << std::endl;
}

/**
 * 读取超级块
 */
void FileSystem::readSuperBlock()
{
    SuperBlock superBlock = {};
    // 读取超级块
    fr.seekg(superBlockStartPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    if (fr.bad())
    {
        std::cerr << "Error: readSuperBlock---Failed to read super block." << std::endl;
    }
    // 打印超级块信息
    std::cout << "Read SuperBlock:" << std::endl;
    std::cout << "\tSize: " << superBlock.superBlockSize << std::endl;
    std::cout << "\tStart position: " << superBlock.superBlockPos << std::endl;
    std::cout << "\tInode bitmap start position: " << superBlock.iNodeBitmapPos << std::endl;
    std::cout << "\tBlock bitmap start position: " << superBlock.blockBitmapPos << std::endl;
    std::cout << "\tInode start position: " << superBlock.iNodeStartPos << std::endl;
    std::cout << "\tBlock start position: " << superBlock.blockStartPos << std::endl;
    std::cout << "\tBlock size: " << superBlock.blockSize << std::endl;
    std::cout << "\tBlock number: " << superBlock.blockNum << std::endl;
    std::cout << "\tFree block number: " << superBlock.freeBlockNum << std::endl;
    std::cout << "\tFree block stack top: " << superBlock.freeBlockAddr << std::endl;
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
 * 打印当前目录
 */
void FileSystem::printCurrentDir()
{
    std::cout << currentDirName;
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
    superBlock.blockGroupSize = BLOCK_GROUP_SIZE;
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
    INode rootINode{};
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
    DirItem dirItems[16] = {};
    // 初始化第一个目录项为"."
    strcpy(dirItems[0].itemName, ".");
    dirItems[0].iNodeAddr = iNodeAddr;
    // 将目录写回
    fw.seekp(blockAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));

    // 给根目录inode赋值
    // inode号
    rootINode.iNodeNo = 0;
    // 大小
    rootINode.iNodeSize = superBlock.blockSize;
    // 时间
    rootINode.iNodeCreateTime = time(nullptr);
    rootINode.iNodeModifyTime = time(nullptr);
    rootINode.iNodeAccessTime = time(nullptr);
    // 用户名与用户组
    strcpy(rootINode.iNodeOwner, userState.userName);
    strcpy(rootINode.iNodeGroup, userState.userGroup);
    // 链接，目前为1，即初始化时仅链接到当前自己"."
    rootINode.iNodeLink = 1;
    // 设置inode数据项
    rootINode.iNodeBlockPointer[0] = blockAddr;
    for (int i = 1; i < BLOCK_POINTER_NUM; ++i)
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
 * @brief 要求用户自己设定的文件系统的其他配置例如密码、用户、组等
 */
void FileSystem::initSystemConfig()
{
}

/**
 * 创建文件系统
 * @param systemName 文件系统名
 */
void FileSystem::createFileSystem(const std::string& systemName)
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
    currentDirName = "/";

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
    fr.seekg(superBlock.superBlockPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    if (fr.bad())
    {
        std::cerr << "Error: install---Failed to read super block." << std::endl;
        return false;
    }
    // 读取inode位图
    fr.seekg(superBlock.iNodeBitmapPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&iNodeBitmap), sizeof(iNodeBitmap));
    if (fr.bad())
    {
        std::cerr << "Error: install---Failed to read inode bitmap." << std::endl;
        return false;
    }
    // 读取数据块位图
    fr.seekg(superBlock.blockBitmapPos, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&blockBitmap), sizeof(blockBitmap));
    if (fr.bad())
    {
        std::cerr << "Error: install---Failed to read block bitmap." << std::endl;
        return false;
    }
    installed = true;
    // 初始化FileManager
    fileManager = FileManager();
    // 初始化INodeCache
    dirINodeCache = INodeCache(MAX_INODE_CACHE_SIZE);
    // 初始化一致性位图
    iNodeCacheBitmap.reset();
    // 将根目录inode加入缓存
    INode rootINode = {};
    fr.seekg(rootINodeAddr, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&rootINode), sizeof(INode));
    dirINodeCache.addINodeToCache("/", rootINode);
    iNodeCacheBitmap[rootINode.iNodeNo] = true;
    return true;
}

void FileSystem::uninstall()
{
    // 关闭文件
    fr.close();
    fw.close();
    // 重置文件系统
    installed = false;
    // 重置用户状态
    userState = {};
    // 设置char[] userState.userName 为"root"
    strcpy(userState.userName, "root");
    // 设置char[] userState.userGroup 为"root"
    strcpy(userState.userGroup, "root");
    // 重置当前目录
    currentDir = rootINodeAddr;
    // 重置当前用户主目录
    userHomeDir = -1;
    // 重置当前目录名
    currentDirName = "/";
    // 重置主机名
    memset(hostName, 0, sizeof(hostName));
    // 重置根目录inode节点
    rootINodeAddr = -1;
    // 重置superBlock
    superBlock = {};
}

/**
 * 保存文件系统
 */
void FileSystem::save()
{
    std::cout << "Saving file system..." << std::endl;
    // 保存超级块
    fw.seekp(superBlock.superBlockPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    // 保存inode位图
    fw.seekp(superBlock.iNodeBitmapPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&iNodeBitmap), sizeof(iNodeBitmap));
    // 保存数据块位图
    fw.seekp(superBlock.blockBitmapPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&blockBitmap), sizeof(blockBitmap));
    // 清空缓冲区
    fw.flush();
    // printBuffer();
    std::cout << "File system saved successfully." << std::endl;
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
        std::cerr << "Error: load---Failed to load virtual disk. " << filename << std::endl;
        return;
    }
    // 读取文件
    fr.read(diskBuffer, diskBlockSize);
    fr.close();
    fw.open(filename, std::ios::out | std::ios::binary);
    if (!fw.is_open())
    {
        std::cerr << "Error: load---Failed to load virtual disk. " << filename << std::endl;
        return;
    }
    // 将缓冲区写回
    fw.write(diskBuffer, diskBlockSize);
    fw.flush();
    // 再次读打开文件
    fr.open(filename, std::ios::in | std::ios::binary);
    if (!fr.is_open())
    {
        std::cerr << "Error: load---Failed to load virtual disk. " << filename << std::endl;
        return;
    }
    // printBuffer();
    // 获取主机名
    getHostName();
    // 设置根目录inode
    rootINodeAddr = iNodeStartPos;
    // 设置当前目录inode
    currentDir = rootINodeAddr;
    // 设置当前目录名
    currentDirName = "/";

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
 * 文件系统内执行命令
 * @param command
 */
void FileSystem::executeInFS(const std::string& command)
{
    std::cout << "Executing command: " << command << std::endl;
    // 解析为数组，前面一个是命令，后面一个是参数，仅拆分第一个空格，不采用sstream
    std::vector<std::string> args;
    // 首先找到第一个空格
    int pos = command.find_first_of(' ');
    // 如果没有空格
    if (pos == std::string::npos)
    {
        args.push_back(command);
        // 再加一个空字符串
        args.emplace_back("");
    }
    else
    {
        // 如果有空格，那么分割
        args.push_back(command.substr(0, pos));
        args.push_back(command.substr(pos + 1));
    }

    // 如果参数[0]为"help"或者参数[1]为"--help"
    if (args[0] == "help" || args[1] == "--help")
    {
        Helper::showHelpInformation(args[0]);
    }
    // 判断命令
    // 如果是pwd命令
    if (args[0] == "pwd")
    {
        std::cout << currentDirName << std::endl;
    }
    // 如果是super命令
    else if (args[0] == "super")
    {
        printSuperBlock();
    }
    // 如果是info命令
    else if (args[0] == "info")
    {
        printSystemInfo();
    }
    // 格式化文件系统
    else if (args[0] == "format")
    {
        formatFileSystem();
    }
    // 如果是cd命令
    else if (args[0] == "cd")
    {
        int iNodeAddr = currentDir;
        std::string temp = currentDirName;
        if (!changeDir(args[1], currentDir, currentDirName))
        {
            currentDir = iNodeAddr;
            currentDirName = temp;
        }
    }
    // 如果是ls命令
    else if (args[0] == "ls")
    {
        listDir(args[1]);
    }
    // 如果是mkdir命令
    else if (args[0] == "mkdir")
    {
        makeDir(args[1]);
    }
    // 如果是rmdir命令
    else if (args[0] == "rmdir")
    {
        removeDir(args[1]);
    }
    // 如果是create命令
    else if (args[0] == "touch" || args[0] == "create")
    {
        createFile(args[1]);
    }
    // 如果是rm命令
    else if (args[0] == "rm" || args[0] == "delete")
    {
        deleteFile(args[1]);
    }
    // 如果是cat命令
    else if (args[0] == "cat")
    {
        catFile(args[1]);
    }
    // 如果是echo命令
    else if (args[0] == "echo")
    {
        echoFile(args[1]);
    }
    // 如果是save命令
    else if (args[0] == "save")
    {
        save();
    }
    // 如果是open命令
    else if (args[0] == "open")
    {
        openWithFilename(args[1]);
    }
    // 如果是close命令
    else if (args[0] == "close")
    {
        closeFile(args[1]);
    }
    // write
    else if (args[0] == "write")
    {
        writeFile(args[1]);
    }
    // read
    else if (args[0] == "read")
    {
        readFile(args[1]);
    }
    // seek
    else if (args[0] == "seek")
    {
        seekWithFd(args[1]);
    }
    else
    {
        // 不识别的命令
        std::cerr << "Error: Command not found." << std::endl;
    }
}

/**
 * 打印inode信息
 * @param node
 */
void FileSystem::printINode(const INode& node)
{
    std::cout << "INode:" << std::endl;
    std::cout << "\tINode number: " << node.iNodeNo << std::endl;
    std::cout << "\tINode size: " << node.iNodeSize << std::endl;
    std::cout << "\tINode create time: " << node.iNodeCreateTime << std::endl;
    std::cout << "\tINode modify time: " << node.iNodeModifyTime << std::endl;
    std::cout << "\tINode access time: " << node.iNodeAccessTime << std::endl;
    std::cout << "\tINode owner: " << node.iNodeOwner << std::endl;
    std::cout << "\tINode group: " << node.iNodeGroup << std::endl;
    std::cout << "\tINode link: " << node.iNodeLink << std::endl;
    std::cout << "\tINode mode: " << node.iNodeMode << std::endl;
    std::cout << "\tINode block pointer: ";
    for (int i = 0; i < BLOCK_POINTER_NUM; i++)
    {
        std::cout << node.iNodeBlockPointer[i] << " ";
    }
    std::cout << std::endl;
}

/**
 * 打印superBlock信息
 */
void FileSystem::printSuperBlock()
{
    // 空出一行
    std::cout << std::endl;
    // 超级块相关信息
    std::cout << "SuperBlock:" << std::endl;
    std::cout << "\tSize: " << superBlock.superBlockSize << std::endl;
    std::cout << "\tStart position: " << superBlock.superBlockPos << std::endl;
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
}

/**
 * 打印系统信息
 */
void FileSystem::printSystemInfo()
{
    // 空出一行
    std::cout << std::endl;
    // 打印系统信息
    std::cout << "System Information:" << std::endl;
    // 主机名
    std::cout << "\tHost name: " << hostName << std::endl;
    // 用户信息
    std::cout << "User Information:" << std::endl;
    // 用户名
    std::cout << "\tUser name: " << userState.userName << std::endl;
    // 用户组
    std::cout << "\tUser group: " << userState.userGroup << std::endl;
    // 是否登录
    std::cout << "\tLogin: " << (userState.isLogin ? "Yes" : "No") << std::endl;
    // 当前目录
    std::cout << "\tCurrent directory: " << currentDirName << std::endl;
    // 用户主目录
    std::cout << "\tUser home directory: " << userHomeDir << std::endl;
}

/**
 * 打印用户和主机名
 */
void FileSystem::printUserAndHostName()
{
    std::cout << userState.userName << "@" << hostName << ":";
}

/**
 * 清屏
 */
void FileSystem::clearScreen()
{
    // 如果是windows系统
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
    system("cls");
#else
    system("clear");
#endif
}

/*-------------------inode基础操作-------------------*/

/**
 * 根据dirName和inodeName拼出绝对路径
 */
std::string FileSystem::getAbsolutePath(const std::string& dirName, const std::string& INodeName)
{
    // 如果INodeName为空或者为"."，那么直接为当前目录
    if (INodeName.empty() || INodeName == ".")
    {
        return dirName;
    }
    // 如果为".."，那么需要找到上一个"/"的位置
    else if (INodeName == "..")
    {
        // 如果当前目录为根目录
        if (dirName == "/")
        {
            return "/";
        }
        // 找到上一个"/"的位置
        int pos = dirName.find_last_of('/');
        // 如果找不到"/"，那么直接返回当前目录
        if (pos == -1)
        {
            std::cerr << "Error: findINodeInDir---No parent directory." << std::endl;
            return dirName;
        }
        // 如果找到了"/"，那么截取字符串
        return dirName.substr(0, pos);
    }
    else
    {
        // 如果是根目录
        if (dirName == "/")
        {
            return dirName + INodeName;
        }
        else
        {
            return dirName + "/" + INodeName;
        }
    }
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
    fw.flush();
    // 更新inode位图
    iNodeBitmap[iNodeIndex] = false;
    // 更新超级块
    superBlock.freeINodeNum++;
    fw.seekp(superBlockStartPos, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&superBlock), sizeof(SuperBlock));
    fw.flush();
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
    unsigned int groupTop = (superBlock.freeBlockNum - 1) % superBlock.blockGroupSize;
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
    if (blockAddr < blockStartPos || blockAddr >= blockStartPos + superBlock.blockNum * superBlock.blockSize || (
        blockAddr - blockStartPos) % BLOCK_SIZE != 0)
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
    unsigned int groupTop = (superBlock.freeBlockNum - 1) % superBlock.blockGroupSize;
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
 * 查找缓存中的inode
 * @param absolutePath
 * @param type inode类型
 */
INode* FileSystem::findINodeInCache(const std::string& absolutePath, int type)
{
    INode* nextINode = nullptr;
    // 如果是目录
    if (type == DIR_TYPE)
    {
        // 查找inode
        nextINode = dirINodeCache.findINodeByFileName(absolutePath);
        if (nextINode != nullptr)
        {
            // 还要再判断一下是否被释放
            if (!iNodeBitmap[nextINode->iNodeNo])
            {
                // 更新缓存
                dirINodeCache.removeINodeFromCache(absolutePath);
                // 设置一致性
                iNodeCacheBitmap[nextINode->iNodeNo] = false;
                // 而如果是被释放了，那么就返回一个空的INode
                return nullptr;
            }
            // 如果与磁盘不一致，那么就需要更新缓存
            if (!iNodeCacheBitmap[nextINode->iNodeNo])
            {
                // 从磁盘中读取
                int iNodeAddr = superBlock.iNodeStartPos + nextINode->iNodeNo * superBlock.iNodeSize;
                fr.seekg(iNodeAddr, std::ios::beg);
                fr.read(reinterpret_cast<char*>(nextINode), sizeof(INode));
                // 更新缓存
                dirINodeCache.addINodeToCache(absolutePath, *nextINode);
                iNodeCacheBitmap[nextINode->iNodeNo] = true;
            }
            return nextINode;
        }
    }
    else if (type == FILE_TYPE)
    {
        // 在FileManager中查找
        nextINode = fileManager.iNodeCache->findINodeByFileName(absolutePath);
        if (nextINode != nullptr)
        {
            // 还要再判断一下是否被释放
            if (!iNodeBitmap[nextINode->iNodeNo])
            {
                // 更新缓存
                fileManager.iNodeCache->removeINodeFromCache(absolutePath);
                iNodeCacheBitmap[nextINode->iNodeNo] = false;
                // 而如果是被释放了，那么就返回一个空的INode
                return nullptr;
            }
            // 如果与磁盘不一致，那么就需要更新缓存
            if (!iNodeCacheBitmap[nextINode->iNodeNo])
            {
                // 从磁盘中读取
                int iNodeAddr = superBlock.iNodeStartPos + nextINode->iNodeNo * superBlock.iNodeSize;
                fr.seekg(iNodeAddr, std::ios::beg);
                fr.read(reinterpret_cast<char*>(nextINode), sizeof(INode));
                // 更新缓存
                fileManager.iNodeCache->addINodeToCache(absolutePath, *nextINode);
                iNodeCacheBitmap[nextINode->iNodeNo] = true;
            }
            return nextINode;
        }
    }
    return nextINode;
}

/**
 * 根据目录INode查找目录下指定的文件/目录INode
 * @param dirINode 待查找的目录
 * @param dirName 目录名
 * @param INodeName 需在目录中查找的项
 * @param type 类型
 * @return resINode
 */
INode* FileSystem::findINodeInDir(INode& dirINode, const std::string& dirName, const std::string& INodeName,
                                  int type)
{
    // 如果不是目录
    if (!(dirINode.iNodeMode & DIR_TYPE))
    {
        std::cerr << "Error: findINodeInDir---Not a directory." << std::endl;
        return nullptr;
    }
    /* 根据type首先查找缓存 */
    std::string absolutePath = getAbsolutePath(dirName, INodeName);

    INode* nextINode = findINodeInCache(absolutePath, type);
    // 如果找到了
    if (nextINode != nullptr)
    {
        return nextINode;
    }

    /*-------------------查找目录项-------------------*/
    // INode已有，直接查找所有目录项
    // 读取目录项
    DirItem dirItems[16] = {};
    int cnt = dirINode.iNodeLink;
    // 最多查找144个目录项
    for (int i = 0; i < 144 && i < cnt;)
    {
        // 如果直接块指针为空，那么直接跳过
        if (dirINode.iNodeBlockPointer[i / 16] == -1)
        {
            i += 16;
            continue;
        }
        // 读取目录项
        int dirItemBlockAddr = dirINode.iNodeBlockPointer[i / 16];
        fr.seekg(dirItemBlockAddr, std::ios::beg);
        fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
        // 遍历目录项
        for (int j = 0; j < 16; j++)
        {
            // 如果目录项为空，那么直接跳过
            if (dirItems[j].iNodeAddr == -1)
            {
                continue;
            }
            // 如果找到了目录项
            if (strcmp(dirItems[j].itemName, INodeName.c_str()) == 0)
            {
                // 判断与type是否匹配
                INode* resINode = new INode();
                fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                fr.read(reinterpret_cast<char*>(resINode), sizeof(INode));
                if ((resINode->iNodeMode & type) == type)
                {
                    // 将找到的INode加入缓存
                    if (type == DIR_TYPE)
                    {
                        dirINodeCache.addINodeToCache(absolutePath, *resINode);
                        iNodeCacheBitmap[resINode->iNodeNo] = true;
                    }
                    else if (type == FILE_TYPE)
                    {
                        fileManager.iNodeCache->addINodeToCache(absolutePath, *resINode);
                        iNodeCacheBitmap[resINode->iNodeNo] = true;
                    }
                    return resINode;
                }
                else
                {
                    std::cerr << "Error: findINodeInDir---Not a " << (type == DIR_TYPE ? "directory." : "file.") <<
                        std::endl;
                    return nullptr;
                }
            }
        }
        i += 16;
    }

    // 如果没有找到
    return nullptr;
}

/**
 * 根据目录INode查找目录下空闲的目录项，附带查看是否有重名目录项
 * @param dirINode 目录INode
 * @param itemName 目录项名
 * @param itemType 目录项类型
 * @return 空闲目录项 FreeDirItemIndex
 */
FreeDirItemIndex FileSystem::findFreeDirItem(const INode& dirINode, const std::string& itemName, int itemType)
{
    int accessPermission = calculatePermission(dirINode);
    // 如果不是目录
    if (!(dirINode.iNodeMode & DIR_TYPE))
    {
        std::cerr << "Error: findFreeDirItem---Not a directory." << std::endl;
        return FreeDirItemIndex{-1, -1};
    }
    // 如果没有进入权限（不包括root用户），则报错
    if (((dirINode.iNodeMode >> accessPermission >> 2) & 1) == 0 && strcmp(userState.userName, "root") != 0)
    {
        std::cerr << "Error: Permission denied." << std::endl;
        return FreeDirItemIndex{-1, -1};
    }
    // 读取目录项
    DirItem dirItems[16] = {};
    bool checkSameName = false;
    if (itemName != "")
    {
        checkSameName = true;
    }
    FreeDirItemIndex freeDirItemIndex = {-1, -1};
    // 最多查找144个目录项
    for (int i = 0; i < 144;)
    {
        // 如果直接块指针为空，那么设置为空闲目录项，如果不需要继续检查重名，那么直接返回
        if (dirINode.iNodeBlockPointer[i / 16] == -1)
        {
            // 如果之前没有找到空闲目录项，那么设置为空闲目录项
            if (freeDirItemIndex.dirItemIndex == -1)
            {
                freeDirItemIndex.dirItemIndex = i;
                freeDirItemIndex.dirItemInnerIndex = 0;
            }
            if (!checkSameName)
            {
                return freeDirItemIndex;
            }
            i += 16;
            continue;
        }
        // 读取目录项
        int dirItemBlockAddr = dirINode.iNodeBlockPointer[i / 16];
        fr.seekg(dirItemBlockAddr, std::ios::beg);
        fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
        // 遍历目录项
        for (int j = 0; j < 16; j++)
        {
            // 如果需要检查重名、重名且类型匹配
            if (checkSameName && strcmp(dirItems[j].itemName, itemName.c_str()) == 0)
            {
                // 如果类型匹配
                INode tempINode{};
                fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                fr.read(reinterpret_cast<char*>(&tempINode), sizeof(INode));
                if ((tempINode.iNodeMode & itemType) == itemType)
                {
                    std::cerr << "Error: findFreeDirItem---File or directory already exists." << std::endl;
                    return FreeDirItemIndex{-1, -1};
                }
            }
            // 如果目录项为空，那么设置为空闲目录项
            if (dirItems[j].itemName[0] == '\0')
            {
                // 如果之前没有找到空闲目录项，那么设置为空闲目录项
                if (freeDirItemIndex.dirItemIndex == -1)
                {
                    freeDirItemIndex.dirItemIndex = i;
                    freeDirItemIndex.dirItemInnerIndex = j;
                }
                // 如果不需要继续检查重名，那么直接返回
                if (!checkSameName)
                {
                    return freeDirItemIndex;
                }
            }
        }
        i += 16;
    }

    // TODO:查找一级间接块，一个一级间接块大小为512字节，一个一级间接块可以存放128个目录项

    // 如果没有找到
    return freeDirItemIndex;
}

/**
 * 查找在某个目录INode中空闲的目录项
 * @param dirINode 目录INode
 * @return 空闲目录项 FreeDirItemIndex
 */
FreeDirItemIndex FileSystem::findFreeDirItem(const INode& dirINode)
{
    // 调用重载函数
    return findFreeDirItem(dirINode, "", -1);
}

/**
 * 清空inode下所有数据块
 * @param iNode inode
 */
void FileSystem::clearINodeBlocks(INode& iNode)
{
    // 如果是目录
    if (iNode.iNodeMode & DIR_TYPE)
    {
        // 读取目录项
        DirItem dirItems[16] = {};
        // 最多查找144个目录项
        for (int i = 0; i < 144;)
        {
            // 如果直接块指针为空，那么直接跳过
            if (iNode.iNodeBlockPointer[i / 16] == -1)
            {
                i += 16;
                continue;
            }
            // 读取目录项
            int dirItemBlockAddr = iNode.iNodeBlockPointer[i / 16];
            fr.seekg(dirItemBlockAddr, std::ios::beg);
            fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
            // 遍历目录项
            for (int j = 0; j < 16; j++)
            {
                // 如果目录项为空，那么直接跳过
                if (dirItems[j].iNodeAddr == -1)
                {
                    continue;
                }
                // 读取目录项对应的inode
                INode nextINode{};
                fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                fr.read(reinterpret_cast<char*>(&nextINode), sizeof(INode));
                // 递归清空
                clearINodeBlocks(nextINode);
                // 释放inode
                freeINode(dirItems[j].iNodeAddr);
                // 更新目录项
                dirItems[j].iNodeAddr = -1;
                // 更新iNode
                iNode.iNodeLink--;
            }
            // 更新目录项
            fw.seekp(dirItemBlockAddr, std::ios::beg);
            fw.write(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
            fw.flush();
            // 更新i
            i += 16;
        }
        // 写回inode
        int iNodeAddr = superBlock.iNodeStartPos + iNode.iNodeNo * superBlock.iNodeSize;
        fw.seekp(iNodeAddr, std::ios::beg);
        fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
        fw.flush();
        // 设置缓存一致性
        iNodeCacheBitmap[iNode.iNodeNo] = false;
    }
    else
    {
        // 释放数据块
        for (int i = 0; i < 9; i++)
        {
            if (iNode.iNodeBlockPointer[i] != -1)
            {
                freeBlock(iNode.iNodeBlockPointer[i]);
                // 更新inode
                iNode.iNodeBlockPointer[i] = -1;
            }
        }
        // 释放一级间接块
        if (iNode.iNodeBlockPointer[9] != -1)
        {
            int block[128] = {};
            fr.seekg(iNode.iNodeBlockPointer[9], std::ios::beg);
            fr.read(reinterpret_cast<char*>(&block), sizeof(block));
            for (int i = 0; i < 128; i++)
            {
                if (block[i] != -1)
                {
                    freeBlock(block[i]);
                }
            }
            freeBlock(iNode.iNodeBlockPointer[9]);
            // 更新inode
            iNode.iNodeBlockPointer[9] = -1;
        }
        // 释放二级间接块,不考虑先
    }
}

/**
 * 计算当前用户对于某个inode的权限移位
 * @param iNode
 * @return
 */
int FileSystem::calculatePermission(const INode& iNode)
{
    if (strcmp(userState.userName, iNode.iNodeOwner) == 0)
    {
        return 6;
    }
    if (strcmp(userState.userGroup, iNode.iNodeGroup) == 0)
    {
        return 3;
    }
    return 0;
}

/*-------------------目录操作-------------------*/

/**
 * 改变当前目录，进入目录
 * @param arg cd命令参数
 */
bool FileSystem::changeDir(const std::string& arg, int& currentDir, std::string& currentDirName)
{
    // 如果参数为空或者"~"，则返回用户目录
    if (arg.empty()||arg=="~")
    {
        // 如果用户主目录为空，那么就是根目录
        if (userHomeDir == -1)
        {
            currentDir = rootINodeAddr;
            currentDirName = "/";
        }
        else
        {
            currentDir = userHomeDir;
            currentDirName = "/home/" + std::string(userState.userName);
        }
        return true;
    }

    if (arg.at(0)=='/')
        // 如果是绝对路径
    {
        // 设置当前目录为根目录然后递归查找
        currentDir = rootINodeAddr;
        currentDirName = "/";
        // 递归查找
        std::string remainPath = arg.substr(1);
        return changeDir(remainPath, currentDir, currentDirName);
    }
    // 如果是相对路径
    // 解析需要匹配的目录名和剩余路径，如果arg后有"/"那么第一个"/"前的是目录名，后面的是剩余路径
    std::string dirName;
    std::string remainPath;
    // 如果arg后有"/"
    if (arg.find('/') != std::string::npos)
    {
        // 找到第一个"/"的位置
        int pos = arg.find('/');
        // 获取目录名
        dirName = arg.substr(0, pos);
        // 获取剩余路径
        remainPath = arg.substr(pos + 1);
    }
    else
    {
        // 如果没有"/"，那么就是目录名
        dirName = arg;
        remainPath = "";
    }
    // 如果目录名超出目录项的最大长度
    if (dirName.size() > MAX_DIRITEM_NAME_LEN)
    {
        std::cerr << "Error: cd---Invalid directory name." << std::endl;
        return false;
    }
    // 取出当前目录的inode
    INode currentINode{};
    fr.seekg(currentDir, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&currentINode), sizeof(INode));
    // 查找inode下的目录项
    // cnt是目录项数目
    int cnt = currentINode.iNodeLink;
    // 判断当前用户权限
    int accessPermission = calculatePermission(currentINode);
    // 使用findINodeInDir查找目录项
    INode* nextINode = findINodeInDir(currentINode, currentDirName, dirName, DIR_TYPE);
    // 如果找到了目录，即nextINode不为空，而不是no为-1
    if (nextINode != nullptr)
    {
        // 查验权限
        if (((nextINode->iNodeMode >> accessPermission >> 2) & 1) == 0 && strcmp(userState.userName, "root") != 0)
        {
            std::cerr << "Error: Permission denied." << std::endl;
            return false;
        }
        // 更新当前目录inode
        currentDir = superBlock.iNodeStartPos + nextINode->iNodeNo * superBlock.iNodeSize;
        // 更新当前目录名
        // 更新当前目录名，分情况讨论，如果是"."/".."/"name"
        if (strcmp(dirName.c_str(), ".") == 0)
        {
            // 不需要改变
            // currentDirName = currentDirName;
        }
        else if (strcmp(dirName.c_str(), "..") == 0)
        {
            // 如果是根目录，那么就不变
            if (currentDirName == "/")
            {
                currentDirName = "/";
            }
            else
            {
                // 如果不是根目录，那么就找到上一级目录
                int pos = currentDirName.find_last_of('/');
                // 如果上级目录是根目录，那么就直接是根目录
                if (pos == 0)
                {
                    currentDirName = "/";
                }
                else
                {
                    // 否则就是上级目录
                    currentDirName = currentDirName.substr(0, pos);
                }
            }
        }
        else
        {
            // 如果当前目录是根目录，那么就不加"/"
            if (currentDirName == "/")
            {
                currentDirName += dirName;
            }
            else
            {
                currentDirName += "/" + dirName;
            }
        }
        // 如果剩余路径不为空，那么继续递归
        if (!remainPath.empty())
        {
            return changeDir(remainPath, currentDir, currentDirName);
        }
        return true;
    }

    // 如果找不到目录项
    std::cerr << "Error: cd---No such directory." << std::endl;
    return false;
}

/**
 * 创建目录的递归函数
 * @param pFlag 是否为-p参数
 * @param dirName 目录名
 * @param inodeAddr inode地址
 */
bool FileSystem::mkdirHelper(bool pFlag, const std::string& dirName, int inodeAddr)
{
    /*-------------------参数检查-------------------*/
    // 如果目录名为空，那么报错
    if (dirName.empty())
    {
        std::cerr << "Error: mkdir---Invalid directory name." << std::endl;
        std::cerr << "Usage: mkdir [-p] <directory>" << std::endl;
        std::cerr.flush();
        return false;
    }
    // 如果inode地址不合法，那么报错
    if (inodeAddr < iNodeStartPos || inodeAddr >= iNodeStartPos + superBlock.iNodeNum * superBlock.iNodeSize || (inodeAddr - iNodeStartPos) % superBlock.iNodeSize != 0)
    {
        std::cerr << "Error: mkdir---Invalid inode address." << std::endl;
        std::cerr.flush();
        return false;
    }

    /*-------------------解析目录名-------------------*/
    // 解析需要匹配的目录名和剩余路径，如果arg后有"/"那么第一个"/"前的是目录名，后面的是剩余路径
    std::string nextDirName;
    std::string remainPath;
    // 如果arg后有"/"
    if (dirName.find('/') != std::string::npos)
    {
        // 找到第一个"/"的位置
        int pos = dirName.find('/');
        // 获取目录名
        nextDirName = dirName.substr(0, pos);
        // 获取剩余路径
        remainPath = dirName.substr(pos + 1);
    }
    else
    {
        // 如果没有"/"，那么就是目录名
        nextDirName = dirName;
        remainPath = "";
    }
    // 如过需要匹配的目录名超出长度，那么报错
    if (nextDirName.size() >= MAX_DIRITEM_NAME_LEN)
    {
        std::cerr << "Error: mkdir---Directory name too long." << std::endl;
        std::cerr << "Directory name should be less than " << MAX_DIRITEM_NAME_LEN << " characters." << std::endl;
        std::cerr.flush();
        return false;
    }
    // 如果剩余路径为空，那么就是最后一个目录，是需要被创建的目录
    bool isLastDir = remainPath.empty();
    /*-------------------读取inode-------------------*/
    // 读取inode
    INode iNode = {};
    fr.seekg(inodeAddr, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&iNode), sizeof(INode));
    // 读取目录项
    DirItem dirItems[16] = {};
    // 目录项数目，这里仅仅只是表示将要创建一个新的目录项
    int cnt = iNode.iNodeLink+1;
    /*-------------------遍历目录项获取创建位置-------------------*/
    int emptyDirItemBlockIndex = -1;
    int emptyDirItemIndex = -1;
    // 读取目录项，此处仅判断直接块指针
    for (int i = 0; i<cnt && i<144;)
    {
        int dirBlockNo = i/16;
        // 如果直接块指针为空，那么直接跳过
        if (iNode.iNodeBlockPointer[dirBlockNo] == -1)
        {
            // 如果直接块没有被使用，那么记录下来
            if (emptyDirItemBlockIndex == -1)
            {
                emptyDirItemBlockIndex = dirBlockNo;
                emptyDirItemIndex = 0;
            }
            i+=16;
            continue;
        }
        // 读取目录项
        int dirItemBlockAddr = iNode.iNodeBlockPointer[dirBlockNo];
        fr.seekg(dirItemBlockAddr, std::ios::beg);
        fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
        /**
         * 遍历目录项
         * 在遍历的过程中，如果isLastDir为true，那么我们就需要来创建最终目录项，而如果这时有重名的目录项，那么就报错
         * 另外，如果isLastDir为false，那么我们就需要递归查找，如果找到了，那么就继续递归，否则如果pFlag为true，那么就创建父级目录继续递归
         * 可以优化的是，如果pFlag为true，如果目录项不存在，在递归的过程中，我们可以直接创建目录项，而不需要再次查找
         * 需要注意的是，在查找过程中需要记录第一个空闲的目录项，包括在第几个目录块中，以及在目录块中的第几个目录项
         */
        for (int j=0; j<16; j++)
        {
            // 如果isLastDir为true，那么我们就需要来创建最终目录项
            if (isLastDir)
            {
                // 如果目录项已经存在
                if (strcmp(dirItems[j].itemName, nextDirName.c_str()) == 0)
                {
                    // 读入该目录项的inode，判断如果是目录，那么就需要报错
                    INode temp = {};
                    fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                    fr.read(reinterpret_cast<char*>(&temp), sizeof(INode));
                    if (temp.iNodeMode & DIR_TYPE)
                    {
                        std::cerr << "Error: mkdir---Directory already exists." << std::endl;
                        std::cerr.flush();
                        return false;
                    }
                }
            }
            else
            {
                // 如果isLastDir为false，那么我们就需要递归查找
                // 如果目录项已经存在，那么直接递归
                if (strcmp(dirItems[j].itemName, nextDirName.c_str()) == 0)
                {
                    // 读入该目录项的inode
                    INode temp = {};
                    fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                    fr.read(reinterpret_cast<char*>(&temp), sizeof(INode));
                    // 如果是目录，那么继续递归
                    if (temp.iNodeMode & DIR_TYPE)
                    {
                        int mode = calculatePermission(temp);
                        // 如果没有进入权限（不包括root用户），则报错
                        if (((temp.iNodeMode >> mode >> 2) & 1) == 0 && strcmp(userState.userName, "root") != 0)
                        {
                            std::cerr << "Error: Permission denied." << std::endl;
                            return false;
                        }
                        // 递归
                        return mkdirHelper(pFlag, remainPath, dirItems[j].iNodeAddr);
                    }
                }
            }
            // 判断目录项的文件名是否为空，如果为空，那么就是空目录项，可以使用
            if (dirItems[j].itemName[0] == '\0'&&emptyDirItemBlockIndex == -1)
            {
                emptyDirItemBlockIndex = dirBlockNo;
                emptyDirItemIndex = j;
            }
            i++;
        }
    }

    // TODO:二级索引

    /*-------------------创建目录项-------------------*/
    // 如果不是最后一个目录，而且pFlag为false，那么报错
    if (!isLastDir && !pFlag)
    {
        std::cerr << "Error: mkdir---No such directory." << std::endl;
        std::cerr.flush();
        return false;
    }
    // 如果没有空闲目录项，那么报错
    if (emptyDirItemBlockIndex == -1)
    {
        std::cerr << "Error: mkdir---No free directory enrty left on device." << std::endl;
        std::cerr.flush();
        return false;
    }
    // 如果是最后一个目录或者pFlag为true，那么创建目录项
    // 首先找到直接块
    fr.seekg(iNode.iNodeBlockPointer[emptyDirItemBlockIndex], std::ios::beg);
    fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
    // 首先尝试分配inode
    int newINodeAddr = allocateINode();
    if (newINodeAddr == -1)
    {
        std::cerr << "Error: mkdir---Failed to allocate inode." << std::endl;
        std::cerr.flush();
        return false;
    }
    // 创建目录项
    strcpy(dirItems[emptyDirItemIndex].itemName, nextDirName.c_str());
    dirItems[emptyDirItemIndex].iNodeAddr = newINodeAddr;
    // 设置inode
    INode newINode = {};
    // 计算inode编号，inode编号为inode地址减去inode起始地址再除以inode大小
    newINode.iNodeNo = (newINodeAddr - superBlock.iNodeStartPos) / superBlock.iNodeSize;
    // 设置时间
    newINode.iNodeCreateTime = time(nullptr);
    newINode.iNodeModifyTime = time(nullptr);
    newINode.iNodeAccessTime = time(nullptr);
    // 设置用户和用户组
    strcpy(newINode.iNodeOwner, userState.userName);
    strcpy(newINode.iNodeGroup, userState.userGroup);
    // 设置链接数，包括"."和".."
    newINode.iNodeLink = 2;
    // 尝试分配数据块用于存放目录项
    int newBlockAddr = allocateBlock();
    if (newBlockAddr == -1)
    {
        std::cerr << "Error: mkdir---Failed to allocate block." << std::endl;
        std::cerr.flush();
        // 还需要释放inode
        freeINode(newINodeAddr);
        return false;
    }
    // 目录项
    DirItem newDirItems[16] = {};
    // 初始化目录项
    strcpy(newDirItems[0].itemName, ".");
    strcpy(newDirItems[1].itemName, "..");
    newDirItems[0].iNodeAddr = newINodeAddr;
    newDirItems[1].iNodeAddr = inodeAddr;
    // 将新目录项写入刚申请的数据块
    fw.seekp(newBlockAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&newDirItems), sizeof(newDirItems));
    fw.flush();
    // 设置inode数据项
    newINode.iNodeBlockPointer[0] = newBlockAddr;
    // 设置其他数据项为空
    for (int i = 1; i < BLOCK_POINTER_NUM; ++i)
    {
        newINode.iNodeBlockPointer[i] = -1;
    }
    // 设置默认权限
    newINode.iNodeMode = DIR_TYPE | DEFAULT_DIR_MODE;
    newINode.iNodeSize = superBlock.blockSize;
    // 写回new inode
    fw.seekp(newINodeAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&newINode), sizeof(INode));
    fw.flush();
    // 设置new inode缓存一致性为false
    iNodeCacheBitmap[newINode.iNodeNo] = false;
    // 将当前目录项写回
    fw.seekp(iNode.iNodeBlockPointer[emptyDirItemBlockIndex], std::ios::beg);
    fw.write(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
    fw.flush();
    // 更新当前inode
    iNode.iNodeLink++;
    // 写回当前inode
    fw.seekp(inodeAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
    // 清空缓冲区
    fw.flush();
    // 设置缓存一致性为false
    iNodeCacheBitmap[iNode.iNodeNo] = false;
    // 如果不是最后一个目录，那么继续递归
    if (!isLastDir)
    {
        return mkdirHelper(pFlag, remainPath, newINodeAddr);
    }
    return true;
}

/**
 * 创建目录
 * @brief 在指定目录下创建目录，支持-p参数
 * @param arg mkdir命令参数
 */
void FileSystem::makeDir(const std::string& arg)
{
    // arg: [-p] dir
    // dir: 目录名，可为绝对路径或相对路径
    // 首先按照空格分割参数，判断是否有-p参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.empty()||args.size()>2)
    {
        std::cerr << "Error: mkdir---Invalid argument." << std::endl;
        std::cerr << "Usage: mkdir [-p] <directory>" << std::endl;
        std::cerr.flush();
        return;
    }
    bool pFlag = false;
    std::string dirName;
    // 如果参数为"-p"，那么设置pFlag为true
    if (args[0] == "-p")
    {
        // 如果为"-p"，那么参数为2个，否则报错
        if (args.size() == 2)
        {
            pFlag = true;
            dirName = args[1];
        }
        else
        {
            std::cerr << "Error: mkdir---Invalid argument." << std::endl;
            std::cerr << "Usage: mkdir [-p] <directory>" << std::endl;
            std::cerr.flush();
            return;
        }
    }
    else if (args.size() == 1)
    {
        dirName = args[0];
    }

    // 设定inode，如果输入的是绝对路径(dirName以/开头)，那么就从根目录开始，否则从当前目录开始
    int inodeAddr;
    if (dirName.at(0) == '/')
    {
        inodeAddr = rootINodeAddr;
        // 去掉第一个字符"/"
        dirName = dirName.substr(1);
    }
    else
    {
        inodeAddr = currentDir;
    }
    // 如果-p参数为true，那么允许递归创建目录，否则如果父级目录不存在，那么报错
    // 使用mkdirHelper函数递归创建目录
    if (mkdirHelper(pFlag, dirName, inodeAddr))
    {
        std::cout << "Directory created successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: mkdir---Failed to create directory." << std::endl;
        std::cerr.flush();
    }
}

/**
 * 删除目录的递归函数
 * @param pFlag 是否为-p参数
 * @param dirName 目录名
 * @param inodeAddr inode地址
 */
bool FileSystem::rmdirHelper(bool ignore, bool pFlag, const std::string& dirName, int inodeAddr)
{
    /*-------------------参数检查-------------------*/
    // 如果目录名为空，那么报错
    if (dirName.empty())
    {
        std::cerr << "Error: rmdir---Invalid directory name." << std::endl;
        return false;
    }
    // 如果inode地址不合法，那么报错
    if (inodeAddr < iNodeStartPos || inodeAddr >= iNodeStartPos + superBlock.iNodeNum * superBlock.iNodeSize || (
        inodeAddr - iNodeStartPos) % superBlock.iNodeSize != 0)
    {
        std::cerr << "Error: rmdir---Invalid inode address." << std::endl;
        return false;
    }

    /*-------------------解析目录名-------------------*/
    // 解析需要匹配的目录名和剩余路径，如果arg后有"/"那么第一个"/"前的是目录名，后面的是剩余路径
    std::string nextDirName;
    std::string remainPath;
    // 如果arg后有"/"
    if (dirName.find('/') != std::string::npos)
    {
        // 找到第一个"/"的位置
        int pos = dirName.find('/');
        // 获取目录名
        nextDirName = dirName.substr(0, pos);
        // 获取剩余路径
        remainPath = dirName.substr(pos + 1);
    }
    else
    {
        // 如果没有"/"，那么就是目录名
        nextDirName = dirName;
        remainPath = "";
    }
    // 如过需要匹配的目录名超出长度，那么报错
    if (nextDirName.size() >= MAX_DIRITEM_NAME_LEN)
    {
        std::cerr << "Error: rmdir---Directory name too long." << std::endl;
        return false;
    }
    // 如果剩余路径为空，那么就是最后一个目录，是需要被删除的目录
    bool isLastDir = remainPath.empty();
    /*-------------------读取inode-------------------*/
    // 读取inode
    INode iNode = {};
    fr.seekg(inodeAddr, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&iNode), sizeof(INode));
    // 读取目录项
    int cnt = iNode.iNodeLink;
    DirItem dirItems[16] = {};
    // 权限
    int accessPermission = calculatePermission(iNode);
    /*-------------------遍历目录项进行删除-------------------*/
    // 读取目录项，此处仅判断直接块指针
    for (int i = 0; i < cnt && i < 144;)
    {
        int dirBlockNo = i / 16;
        // 如果直接块指针为空，那么直接跳过
        if (iNode.iNodeBlockPointer[dirBlockNo] == -1)
        {
            i += 16;
            continue;
        }
        // 读取目录项
        int dirItemBlockAddr = iNode.iNodeBlockPointer[dirBlockNo];
        fr.seekg(dirItemBlockAddr, std::ios::beg);
        fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
        // 遍历目录项,每次遍历16个
        for (int j = 0; j < 16; j++)
        {
            // 目录名相同
            if (strcmp(dirItems[j].itemName, nextDirName.c_str()) == 0)
            {
                // 读取目录项的inode
                INode nextINode = {};
                fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                fr.read(reinterpret_cast<char*>(&nextINode), sizeof(INode));
                // 如果是目录
                if (nextINode.iNodeMode & DIR_TYPE)
                {
                    // 如果没有写入权限
                    if ((nextINode.iNodeMode >> accessPermission >> 1 & 1) == 0 && strcmp(userState.userName, "root") != 0)
                    {
                        std::cerr << "Error: Permission denied." << std::endl;
                        return false;
                    }
                    // 如果是最后一个目录，那么删除
                    if (isLastDir)
                    {
                        // 如果目录不为空，那么报错
                        if (nextINode.iNodeLink > 2 && !ignore)
                        {
                            std::cerr << "Error: rmdir---Directory not empty." << std::endl;
                            return false;
                        }
                        // 如果ignore为true，那么递归删除下面的所有目录项
                        if (ignore)
                        {
                            // TODO: removeAll(dirItems[j].iNodeAddr);
                        }
                        // 删除目录条目
                        strcpy(dirItems[j].itemName, ""); // 释放inode
                        // 释放错误
                        if (!freeINode(dirItems[j].iNodeAddr))
                        {
                            std::cerr << "Error: rmdir---Free inode error." << std::endl;
                            std::cerr.flush();
                            return false;
                        }
                        dirItems[j].iNodeAddr = -1;
                        // 写回目录项
                        fw.seekp(dirItemBlockAddr, std::ios::beg);
                        fw.write(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
                        // 释放数据块
                        if (!freeBlock(nextINode.iNodeBlockPointer[0]))
                        {
                            std::cerr << "Error: rmdir---Free block error but inode was deleted." << std::endl;
                            std::cerr.flush();
                            return false;
                        }
                        // 更新当前inode
                        iNode.iNodeLink--;
                        // 写回当前inode
                        fw.seekp(inodeAddr, std::ios::beg);
                        fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
                        // 清空缓冲区
                        fw.flush();
                        // 设置缓存一致性为false
                        iNodeCacheBitmap[iNode.iNodeNo] = false;
                        return true;
                    }
                    // 如果不是最后一个目录，那么继续递归
                    if (rmdirHelper(ignore, pFlag, remainPath, dirItems[j].iNodeAddr))
                    {
                        // TODO:如果此时有pFlag而子目录已经被删除
                        return true;
                    }
                    return false;
                }
            }
        }
    }
    // TODO: 二级索引
    std::cerr << "Error:rmdir---The directory was not found." << std::endl;
    std::cerr.flush();
    return false;
}

/**
 * 删除目录
 * @param arg rmdir命令参数
 */
void FileSystem::removeDir(const std::string& arg)
{
    // arg: [option] dir
    // [option]: -p --ignore-fail-on-non-empty
    // dir: 目录名，可为绝对路径或相对路径
    // 首先按照空格分割参数，判断是否有-p参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.empty() || args.size() > 3)
    {
        std::cerr << "Error: mkdir---Invalid argument." << std::endl;
        return;
    }
    bool pFlag = false, ignore = false;
    std::string dirName;
    // 解析参数列表，最后一个参数为目录名，前面的参数可能为-p或--ignore-fail-on-non-empty
    for (int i = 0; i < args.size() - 1; ++i)
    {
        if (args[i] == "-p")
        {
            pFlag = true;
        }
        else if (args[i] == "--ignore-fail-on-non-empty")
        {
            ignore = true;
        }
        else
        {
            std::cerr << "Error: rmdir---Invalid argument." << std::endl;
            return;
        }
    }
    dirName = args[args.size() - 1];

    // 设定inode，如果输入的是绝对路径(dirName以/开头)，那么就从根目录开始，否则从当前目录开始
    int inodeAddr;
    if (dirName.at(0) == '/')
    {
        inodeAddr = rootINodeAddr;
        // 去掉第一个字符"/"
        dirName = dirName.substr(1);
    }
    else
    {
        inodeAddr = currentDir;
    }

    // 如果-p参数为true，那么允许递归创建目录，否则如果父级目录不存在，那么报错
    // 使用mkdirHelper函数递归创建目录
    if (rmdirHelper(ignore, pFlag, dirName, inodeAddr))
    {
        std::cout << "Directory deleted successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: rmdir---Failed to delete directory." << std::endl;
    }
}

/**
 * 列出目录
 * @param arg ls命令参数
 */
void FileSystem::listDir(const std::string& arg)
{
    int lsMode = LS_DEFAULT;
    // 如果参数为空，那么列出当前目录
    if (arg.empty())
    {
        listDirByINode(currentDir, lsMode);
        return;
    }
    // 如果参数不为空，那么首先将arg按照空格分割
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为"-l"，那么设置lsMode为LS_L
    if (args.size() > 0 && args[0] == "-l")
    {
        lsMode = LS_L;
    }
    // 如果后面还有参数（大于1），那么最后一个参数为目录名
    if (args.size() > 1)
    {
        // 获取目录inode地址
        std::string dirName = args[args.size() - 1];
        int inodeAddr = currentDir;
        std::string inodePath = currentDirName;
        // 使用changeDir函数获取目录inode地址
        // 使用changeDir函数获取目录inode地址
        if (!changeDir(dirName, inodeAddr, inodePath))
        {
            std::cerr << "Error: ls---No such directory." << std::endl;
            return;
        }
        listDirByINode(inodeAddr, lsMode);
    }
    else
    {
        // 如果目录名为空，那么列出当前目录
        listDirByINode(currentDir, lsMode);
    }

}

/**
 * 根据inode地址列出目录
 * @param inodeAddr inode地址
 * @param lsMode ls命令参数
 */
void FileSystem::listDirByINode(int inodeAddr, int lsMode)
{
    // 读取inode
    INode iNode = {};
    fr.seekg(inodeAddr, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&iNode), sizeof(INode));
    // 读取目录项
    DirItem dirItems[16] = {};
    // 目录项数目
    int cnt = iNode.iNodeLink;
    // 判断文件模式
    // 判断当前用户权限
    int mode = calculatePermission(iNode);
    // 判断读取权限
    if (((iNode.iNodeMode >> mode >> 2)&1)==0)
    {
        std::cerr << "Error: ls---Permission denied." << std::endl;
        return;
    }
    // 读取目录项
    for (int i = 0; i<cnt && i<144;)
    {
        // 如果直接块指针为空，那么直接跳过
        if (iNode.iNodeBlockPointer[i/16] == -1)
        {
            i+=16;
            continue;
        }
        // 读取目录项
        int dirItemBlockAddr = iNode.iNodeBlockPointer[i/16];
        fr.seekg(dirItemBlockAddr, std::ios::beg);
        fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
        // 遍历目录项，需要保证不超过目录项数目
        for (int j=0; j<16&&i<cnt; j++)
        {
            // 读取目录项对应的inode
            INode nextINode = {};
            fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
            fr.read(reinterpret_cast<char*>(&nextINode), sizeof(INode));
            // 如果目录项名称为空，说明这个目录项为空
            if (dirItems[j].itemName[0] == '\0')
            {
                j++;
                continue;
            }
            // 如果采用LS_L模式，那么需要打印详细信息
            if (lsMode == LS_L)
            {
                // 如果是目录
                if (nextINode.iNodeMode & DIR_TYPE)
                {
                    std::cout<<"d";
                }
                else if (nextINode.iNodeMode & FILE_TYPE)
                {
                    std::cout<<"-";
                }
                // 打印权限，按照rwxrwxrwx的顺序
                for (int k=8; k>=0; k--)
                {
                    // 如果有权限
                    if ((nextINode.iNodeMode >> k) & 1)
                    {
                        if (k % 3 == 2) printf("r");
                        if (k % 3 == 1) printf("w");
                        if (k % 3 == 0)	printf("x");
                    }
                    else
                    {
                        std::cout<<"-";
                    }
                }
                std::cout<<" ";
                // 依次打印inode的用户名、用户组、大小、创建时间、修改时间、访问时间、文件名
                std::cout << nextINode.iNodeOwner << " ";
                std::cout << nextINode.iNodeGroup << " ";
                std::cout << nextINode.iNodeSize << " ";
                // 时间按照年月日时分秒的格式打印
                // std::cout<<ctime(&nextINode.iNodeCreateTime)<<" ";
                std::cout << ctime(&nextINode.iNodeModifyTime) << " ";
                // std::cout<<ctime(&nextINode.iNodeAccessTime)<< " ";
                std::cout << dirItems[j].itemName << std::endl;
                std::cout.flush();
            }
            else
            {
                // 打印目录项名称
                std::cout << dirItems[j].itemName << " ";
            }
            i++;
        }
    }
    // 换行
    std::cout << std::endl;
}

/*-------------------文件操作-------------------*/

/**
 * 打开文件，即找到文件的inode，然后将其加入inodeCache，返回inode地址
 * @param dirName 目录名
 * @param dirAddr 目录地址
 * @param fileName 文件名
 */
int FileSystem::openFile(std::string& dirName, int dirAddr, std::string& fileName, int mode)
{
    // TODO:链接文件
    /*-------------------读取目录项-------------------*/
    // 读取上级目录的inode
    INode dirINode = {};
    fr.seekg(dirAddr, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&dirINode), sizeof(INode));
    // 找到文件的inode
    INode* fileINode = findINodeInDir(dirINode, dirName, fileName, FILE_TYPE);
    // 计算权限
    int accessPermission = calculatePermission(dirINode);
    // 如果文件inode地址为-1，那么文件不存在，需要创建
    if (fileINode == nullptr)
    {
        // 如果mode为只读，那么报错
        if (mode == MODE_R)
        {
            std::cerr << "Error: openWithFilename---Cannot open file not exist." << std::endl;
            return -1;
        }
        // 如果没有写入权限，那么报错
        if (((dirINode.iNodeMode >> accessPermission >> 1) & 1) == 0)
        {
            std::cerr << "Error: open---Permission denied." << std::endl;
            return -1;
        }
        // 创建文件
        // 空缓冲区
        // 创建文件
        bool res = createFileHelper(fileName, dirAddr, nullptr, 0);
        if (!res)
        {
            std::cerr << "Error: openWithFilename---Failed to create file." << std::endl;
            return -1;
        }
        // 重新读取inode
        fileINode = findINodeInDir(dirINode, dirName, fileName, FILE_TYPE);
    }
    // 打开文件
    // 如果mode&MODE_R为真，那么判断是否具有读权限
    if (mode & MODE_R && ((fileINode->iNodeMode >> accessPermission >> 2) & 1) == 0)
    {
        std::cerr << "Error: openWithFilename---Permission denied." << std::endl;
        return -1;
    }
    // 如果mode&MODE_W为真，那么判断是否具有写权限
    if (mode & MODE_W && ((fileINode->iNodeMode >> accessPermission >> 1) & 1) == 0)
    {
        std::cerr << "Error: openWithFilename---Permission denied." << std::endl;
        return -1;
    }
    // 调用FileManger的doOpenFile
    bool clear = false;
    std::string absoultePath = getAbsolutePath(dirName, fileName);
    int fd = fileManager.doOpenFile(*fileINode, absoultePath, mode, clear);
    if (fd != -1)
    {
        // 如果clear为true，那么清空文件
        if (clear)
        {
            clearINodeBlocks(*fileINode);
            // 再给文件分配一个新的数据块
            int newBlockAddr = allocateBlock();
            if (newBlockAddr == -1)
            {
                std::cerr << "Error: open---Failed to allocate block." << std::endl;
                return -1;
            }
            // 设置inode的数据块
            fileINode->iNodeBlockPointer[0] = newBlockAddr;
            // 设置inode的大小
            fileINode->iNodeSize = 0;
            // 写回inode
            int inodeAddr = fileINode->iNodeNo * superBlock.iNodeSize + superBlock.iNodeStartPos;
            fw.seekp(inodeAddr, std::ios::beg);
            fw.write(reinterpret_cast<char*>(fileINode), sizeof(INode));
            fw.flush();
            // 设置缓存一致性为false
            iNodeCacheBitmap[fileINode->iNodeNo] = false;
        }
        std::cout << "File opened successfully." << std::endl;
        return fd;
    }
    // 报错
    std::cerr << "Error: openWithFilename---Failed to openWithFilename file." << std::endl;
    return -1;
}

/**
 * 解析open命令，打开文件
 * @param arg open命令参数
 */
void FileSystem::openWithFilename(const std::string& arg)
{
    // arg: file [-m mode] mode为rwa
    // file: 文件名，可为绝对路径或相对路径
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.size()!=1&&args.size()!=3)
    {
        std::cerr << "Error: openWithFilename---Invalid argument." << std::endl;
        return;
    }
    // 文件名
    std::string fileName = args[0];
    int mode = MODE_DEFAULT;
    // 如果参数数为3
    if (args.size() == 3)
    {
        if (args[1] != "-m")
        {
            std::cerr << "Error: openWithFilename---Invalid argument." << std::endl;
            std::cerr << "Usage: open <file> [-m mode]" << std::endl;
            std::cerr << "mode: r, w, rw" << std::endl;
            return;
        }
        // 如果有其他字符，那么报错
        if (args[2].find_first_not_of("rwa") != std::string::npos)
        {
            std::cerr << "Error: openWithFilename---Invalid mode." << std::endl;
            std::cerr << "Usage: open <file> [-m mode]" << std::endl;
            std::cerr << "mode: r, w, a" << std::endl;
            return;
        }
        // 解析mode，查看mode中是否包含r、w、a，如果有则设置mode
        if (args[2].find('r') != std::string::npos)
        {
            mode |= MODE_R;
        }
        if (args[2].find('w') != std::string::npos)
        {
            mode |= MODE_W;
        }
        if (args[2].find('a') != std::string::npos)
        {
            mode |= MODE_A;
        }
    }
    // 首先拆分文件名和目录名
    int inodeAddr = currentDir;
    std::string dirName = currentDirName;
    std::string targetDirName = ".";
    if (fileName.find('/') != std::string::npos)
    {
        int pos = fileName.find_last_of('/');
        targetDirName = fileName.substr(0, pos);
        fileName = fileName.substr(pos + 1);
    }

    // 使用changeDir_函数找到文件的inode，并切换到文件所在目录
    if (changeDir(targetDirName, inodeAddr, dirName))
    {
        // 打开文件
        int fd = openFile(dirName, inodeAddr, fileName, mode);
        if (fd != -1)
        {
            std::cout << "File descriptor: " << fd << std::endl;
            return;
        }
        else
        {
            std::cerr << "Error: openWithFilename---Failed to open file." << std::endl;
            return;
        }
    }
    else
    {
        std::cerr << "Error: openWithFilename---Failed to openWithFilename file." << std::endl;
        return;
    }
}

/**
 * 关闭文件——使用文件描述符
 * @param fd 文件描述符
 */
bool FileSystem::closeWithFd(int fd)
{
    // 调用FileManger的doCloseFile
    if (fileManager.doCloseFile(fd))
    {
        std::cout << "File closed successfully." << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Error: closeWithFd---Failed to closeWithFd file." << std::endl;
        return false;
    }
}

/**
 * 按照文件描述符关闭文件
 * @brief 解析close命令，关闭文件
 * @param arg close命令参数
 */
void FileSystem::closeFile(const std::string& arg)
{
    // close fd
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.size() != 1)
    {
        std::cerr << "Error: closeWithFd---Invalid argument." << std::endl;
        std::cerr << "Usage: close <fd>" << std::endl;
        return;
    }
    // 解析文件描述符
    // 首先判断是否为数字
    if (args[0].find_first_not_of("0123456789") != std::string::npos)
    {
        std::cerr << "Error: close---Invalid file descriptor." << std::endl;
        return;
    }
    int fd = std::stoi(args[0]);
    // 调用closeWithFd函数
    closeWithFd(fd);
}

/**
 * 设置文件指针位置——使用文件描述符，解析seek命令
 */
void FileSystem::seekWithFd(const std::string& arg)
{
    // seek fd offset
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.size() != 2)
    {
        std::cerr << "Error: seekWithFd---Invalid argument." << std::endl;
        std::cerr << "Usage: seek <fd> <offset>" << std::endl;
        return;
    }
    // 判断是否可以转换为数字
    if (args[0].find_first_not_of("0123456789") != std::string::npos || args[1].find_first_not_of("0123456789") !=
        std::string::npos)
    {
        std::cerr << "Error: seekWithFd---Invalid argument." << std::endl;
        std::cerr << "Usage: seek <fd> <offset>" << std::endl;
        return;
    }
    // 解析文件描述符和偏移量
    int fd = std::stoi(args[0]);
    int offset = std::stoi(args[1]);
    // 调用FileManger的doSeekFile
    if (fileManager.setFdOffset(fd, offset))
    {
        std::cout << "File pointer moved successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: seekWithFd---Failed to move file pointer." << std::endl;
    }
}

/**
 * 向某个inode对应文件磁盘块写入内容
 * @param iNode inode
 * @param content 内容
 * @param size 内容大小
 * @param offset 写入偏移量
 */
bool FileSystem::sysWriteFile(INode& iNode, const char* content, unsigned int size, unsigned int offset)
{
    // 计算inode地址
    int inodeAddr = iNode.iNodeNo * superBlock.iNodeSize + superBlock.iNodeStartPos;
    // 判断文件模式
    // 判断当前用户权限
    int accessPermission = calculatePermission(iNode);
    // 判断写入权限
    if (((iNode.iNodeMode >> accessPermission >> 1) & 1) == 0)
    {
        std::cerr << "Error: write---Permission denied." << std::endl;
        return false;
    }
    // 如果偏移量加上写入内容大小超出文件最大大小，那么报错
    if (offset + size > maxFileSize)
    {
        std::cerr << "Error: write---Offset + size exceeds file size." << std::endl;
        return false;
    }
    // 如果写入内容为空，那么报错
    if (content == nullptr)
    {
        std::cerr << "Error: write---Invalid content." << std::endl;
        return false;
    }
    // 如果写入内容大小为0，那么直接返回
    if (size == 0)
    {
        return true;
    }
    // 在某个磁盘块中的offset
    int left_offset = offset % superBlock.blockSize;
    // 起始的磁盘块号
    int start_block = offset / superBlock.blockSize;
    // 每次写入一个磁盘块
    for (int left = size; left > 0;)
    {
        // 超出直接块范围，使用二级索引
        if (start_block >= 9)
        {
            //TODO:需要读取索引块，解析索引表，然后依次写入
            // 读取索引块
            int indexBlockAddr = iNode.iNodeBlockPointer[9];
            if (indexBlockAddr == -1)
            {
                // 如果索引块为空，那么分配一个数据块
                int newIndexBlockAddr = allocateBlock();
                if (newIndexBlockAddr == -1)
                {
                    std::cerr << "Error: write---Failed to allocate block." << std::endl;
                    return false;
                }
                // 向索引块中写入磁盘块地址，全部设置为-1
                int indexTable[128] = {};
                for (int i = 0; i < 128; ++i)
                {
                    indexTable[i] = -1;
                }
                // 写入索引块
                fw.seekp(newIndexBlockAddr, std::ios::beg);
                fw.write(reinterpret_cast<char*>(&indexTable), sizeof(indexTable));
                fw.flush();
                iNode.iNodeBlockPointer[9] = newIndexBlockAddr;
                // 写回inode
                fw.seekp(inodeAddr, std::ios::beg);
                fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
                fw.flush();
                // 设置缓存一致性为false
                iNodeCacheBitmap[iNode.iNodeNo] = false;
                indexBlockAddr = newIndexBlockAddr;
            }
            // 读取索引表
            int indexTable[128] = {};
            fr.seekg(indexBlockAddr, std::ios::beg);
            fr.read(reinterpret_cast<char*>(&indexTable), sizeof(indexTable));
            // TODO:重新计算二级磁盘块号
        }
        else
        {
            // 如果直接块指针为空，那么分配一个数据块
            if (iNode.iNodeBlockPointer[start_block] == -1)
            {
                int newBlockAddr = allocateBlock();
                if (newBlockAddr == -1)
                {
                    std::cerr << "Error: write---Failed to allocate block." << std::endl;
                    return false;
                }
                iNode.iNodeBlockPointer[start_block] = newBlockAddr;
            }
            // 磁盘块地址
            int blockAddr = iNode.iNodeBlockPointer[start_block];
            // 写入磁盘块，从left_offset开始写入，最多写入blockSize-left_offset个字节
            int writeSize = std::min(left, superBlock.blockSize - left_offset);
            // 写入磁盘块
            fw.seekp(blockAddr + left_offset, std::ios::beg);
            fw.write(content + size - left, writeSize);
            fw.flush();
            // 写下一个磁盘块
            start_block++;
            // 更新剩余大小和偏移量
            left -= writeSize;
            left_offset = 0;
        }
    }
    // 更新inode
    iNode.iNodeModifyTime = time(nullptr);
    iNode.iNodeSize = std::max(iNode.iNodeSize, offset + size);
    // 写回inode
    fw.seekp(inodeAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
    fw.flush();
    // 设置缓存一致性为false
    iNodeCacheBitmap[iNode.iNodeNo] = false;
    return true;
}

/**
 * 向指定文件描述符对应文件写入内容
 * @param fd 文件描述符
 * @param content 内容
 * @param size 内容大小
 */
bool FileSystem::writeWithFd(int fd, const char* content, unsigned int size)
{
    // 获取文件描述符
    FileDescriptor* fileDescriptor = fileManager.getFdItem(fd);
    // 如果文件描述符为空，那么报错
    if (fileDescriptor == nullptr)
    {
        std::cerr << "Error: writeWithFd---File not opened." << std::endl;
        return false;
    }
    // 判断是否为写入模式
    if ((fileDescriptor->mode & MODE_W) == 0)
    {
        std::cerr << "Error: writeWithFd---File not opened in write mode." << std::endl;
        return false;
    }
    // 根据inode判断当前用户权限
    INode* iNode = fileDescriptor->iNode;
    int accessPermission = calculatePermission(*iNode);
    // 判断写入权限
    if (((iNode->iNodeMode >> accessPermission >> 1) & 1) == 0)
    {
        std::cerr << "Error: writeWithFd---Permission denied." << std::endl;
        return false;
    }
    // 调用sysWriteFile函数写入文件
    if (sysWriteFile(*iNode, content, size, fileDescriptor->offset))
    {
        // 更新文件描述符的偏移量
        fileManager.setFdOffset(fd, fileDescriptor->offset + size);
        return true;
    }
    return false;
}

/**
 * 解析write命令，写入文件
 * @param arg write命令参数
 */
void FileSystem::writeFile(const std::string& arg)
{
    // write fd content
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.size() != 2)
    {
        std::cerr << "Error: writeWithFd---Invalid argument." << std::endl;
        std::cerr << "Usage: write <fd> <content>" << std::endl;
        return;
    }
    // 解析文件描述符
    // 判断是否可以转换为数字
    if (args[0].find_first_not_of("0123456789") != std::string::npos)
    {
        std::cerr << "Error: writeWithFd---Invalid file descriptor." << std::endl;
        return;
    }
    int fd = std::stoi(args[0]);
    // 获取写入内容
    std::string content = args[1];
    // 调用writeWithFd函数
    if (writeWithFd(fd, content.c_str(), content.size()))
    {
        std::cout << "Write successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: writeWithFd---Failed to write." << std::endl;
    }
}

/**
 * 创建文件的辅助函数
 * @param fileName 文件名
 * @param dirINodeAddr inode地址
 * @return 是否创建成功
 */
bool FileSystem::createFileHelper(const std::string& fileName, int dirINodeAddr, char* content, unsigned int size)
{
    /*-------------------参数检查-------------------*/
    // 如果文件名为空或者超出长度，那么报错
    if (fileName.empty() || fileName.size() >= MAX_DIRITEM_NAME_LEN)
    {
        std::cerr << "Error: create---Invalid file name." << std::endl;
        std::cerr.flush();
        return false;
    }

    // 读取目录inode
    INode iNode = {};
    fr.seekg(dirINodeAddr, std::ios::beg);
    fr.read(reinterpret_cast<char*>(&iNode), sizeof(INode));

    // 读取目录项
    DirItem dirItems[16] = {};
    // 目录项数目，这里仅仅只是表示将要创建一个新的目录项
    int cnt = iNode.iNodeLink + 1;
    /*-------------------遍历目录项获取创建位置-------------------*/
    int emptyDirItemBlockIndex = -1;
    int emptyDirItemIndex = -1;
    // 使用findFreeDirItem函数找到空闲目录项
    FreeDirItemIndex freeDirItemIndex = findFreeDirItem(iNode, fileName, FILE_TYPE);
    emptyDirItemBlockIndex = freeDirItemIndex.dirItemIndex;
    emptyDirItemIndex = freeDirItemIndex.dirItemInnerIndex;

    /*-------------------创建目录项-------------------*/
    // 如果没有空闲目录项或者出现重名
    if (emptyDirItemBlockIndex == -1)
    {
        std::cerr << "Error: createFile---No free dir item or file already exists." << std::endl;
        std::cerr.flush();
        return false;
    }
    // 取出空闲目录项
    fr.seekg(iNode.iNodeBlockPointer[emptyDirItemBlockIndex], std::ios::beg);
    fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
    // 尝试分配inode
    int newINodeAddr = allocateINode();
    if (newINodeAddr == -1)
    {
        std::cerr << "Error: createFile---Failed to allocate inode." << std::endl;
        std::cerr.flush();
        return false;
    }
    // 创建目录项
    strcpy(dirItems[emptyDirItemIndex].itemName, fileName.c_str());
    dirItems[emptyDirItemIndex].iNodeAddr = newINodeAddr;
    // 设置inode
    INode newINode = {};
    // 计算inode编号，inode编号为inode地址减去inode起始地址再除以inode大小
    newINode.iNodeNo = (newINodeAddr - superBlock.iNodeStartPos) / superBlock.iNodeSize;
    // 设置时间
    newINode.iNodeCreateTime = time(nullptr);
    newINode.iNodeModifyTime = time(nullptr);
    newINode.iNodeAccessTime = time(nullptr);
    // 设置用户和用户组
    strcpy(newINode.iNodeOwner, userState.userName);
    strcpy(newINode.iNodeGroup, userState.userGroup);
    // 设置磁盘块
    for (int i = 0; i < BLOCK_POINTER_NUM; ++i)
    {
        newINode.iNodeBlockPointer[i] = -1;
    }
    // 设置链接数
    newINode.iNodeLink = 1;
    // 设置默认权限
    newINode.iNodeMode = FILE_TYPE | DEFAULT_FILE_MODE;
    newINode.iNodeSize = size;
    // 写回new inode
    fw.seekp(newINodeAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&newINode), sizeof(INode));
    fw.flush();
    // 设置缓存一致性为false
    iNodeCacheBitmap[newINode.iNodeNo] = false;
    // 写入内容
    if (size > 0)
    {
        // 写入文件
        if (!sysWriteFile(newINode, content, size, 0))
        {
            std::cerr << "Error: create---Failed to write file." << std::endl;
            std::cerr.flush();
            return false;
        }
    }
    // 如果大小等于0，那么也给它分配一个块
    else if (size == 0)
    {
        int newBlockAddr = allocateBlock();
        if (newBlockAddr == -1)
        {
            std::cerr << "Error: create---Failed to allocate block." << std::endl;
            std::cerr.flush();
            return false;
        }
        newINode.iNodeBlockPointer[0] = newBlockAddr;
        // 写回新磁盘块
        fw.seekp(newBlockAddr, std::ios::beg);
        char empty[superBlock.blockSize] = {'\0'};
        fw.write(empty, sizeof(empty));
        // 写回new inode
        fw.seekp(newINodeAddr, std::ios::beg);
        fw.write(reinterpret_cast<char*>(&newINode), sizeof(INode));
        fw.flush();
        // 设置缓存一致性为false
        iNodeCacheBitmap[newINode.iNodeNo] = false;
    }
    // 将当前目录项写回
    fw.seekp(iNode.iNodeBlockPointer[emptyDirItemBlockIndex], std::ios::beg);
    fw.write(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
    fw.flush();
    // 写回当前inode
    // 更新链接数
    iNode.iNodeLink++;
    fw.seekp(dirINodeAddr, std::ios::beg);
    fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
    // 清空缓冲区
    fw.flush();
    // 设置缓存一致性为false
    iNodeCacheBitmap[iNode.iNodeNo] = false;
    return true;
}

/**
 * 创建文件
 * @param arg create命令参数
 */
void FileSystem::createFile(const std::string& arg)
{
    // arg: file
    // file: 文件名，可为绝对路径或相对路径
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.empty() || args.size() > 2)
    {
        std::cerr << "Error: create---Invalid argument." << std::endl;
        return;
    }
    std::string fileName = args[0];
    char* content = nullptr;
    unsigned int size = 0;
    // 如果有第二个参数，那么是文件内容
    if (args.size() == 2)
    {
        content = const_cast<char*>(args[1].c_str());
        size = args[1].size();
    }
    // 设定inode，如果输入的是绝对路径(fileName以/开头)，那么就从根目录开始，否则从当前目录开始
    int inodeAddr = currentDir;
    std::string dirName = currentDirName;
    std::string targetDirName = ".";
    if (fileName.find('/') != std::string::npos)
    {
        int pos = fileName.find_last_of('/');
        targetDirName = fileName.substr(0, pos);
        fileName = fileName.substr(pos + 1);
    }

    // 使用changeDir函数找到文件的inode，并切换到文件所在目录
    if (changeDir(targetDirName, inodeAddr, dirName))
    {
        // 创建文件
        if (createFileHelper(fileName, inodeAddr, content, size))
        {
            std::cout << "File created successfully." << std::endl;
        }
        else
        {
            std::cerr << "Error: create---Failed to create file." << std::endl;
        }
    }
}

/**
 * 以offset为起始位置读取指定大小的文件内容
 * @param inodeAddr inode地址
 * @param offset 起始位置
 * @param size 读取大小
 */
bool FileSystem::sysReadFile(INode& iNode, unsigned int offset, unsigned int size, char* content)
{
    // 如果读取大小为0，那么直接返回
    if (size == 0)
    {
        return true;
    }
    // 如果偏移量超出文件大小，那么报错
    if (offset + size > iNode.iNodeSize)
    {
        std::cerr << "Error: read---Offset exceeds file size." << std::endl;
        return false;
    }
    // 在某个磁盘块中的offset
    int left_offset = offset % superBlock.blockSize;
    // 起始的磁盘块号
    int start_block = offset / superBlock.blockSize;
    int left = size;
    // 每次读取一个磁盘块
    while (left > 0)
    {
        // 超出直接块范围，使用二级索引
        if (start_block >= 9)
        {
            // TODO:需要读取索引块，解析索引表，然后依次读取
        }
        else // 在直接块中
        {
            // 如果直接块指针为空，那么报错
            if (iNode.iNodeBlockPointer[start_block] == -1)
            {
                std::cerr << "Error: read---Block not allocated." << std::endl;
                return false;
            }
            // 磁盘块地址
            int blockAddr = iNode.iNodeBlockPointer[start_block];
            // 从left_offset开始读取，最多读取blockSize-left_offset个字节
            int readSize = std::min(left, superBlock.blockSize - left_offset);
            // 读取磁盘块，读到content中
            fr.seekg(blockAddr + left_offset, std::ios::beg);
            fr.read(content + size - left, readSize);
            // 更新剩余大小和偏移量
            left -= readSize;
            left_offset = 0;
            start_block++;
        }
    }
    // 如果仍有剩余，那么报错
    if (left > 0)
    {
        std::cerr << "Error: read---Failed to successfully read file." << std::endl;
        return false;
    }
    return true;
}

/**
 * 读取指定文件描述符的文件内容，读到content中
 * @param fd 文件描述符
 * @param content 读取内容
 * @param size 读取大小
 */
bool FileSystem::readWithFd(int fd, char* content, unsigned int size)
{
    // 调用FileManger获取文件描述句柄
    FileDescriptor* fileDescriptor = fileManager.getFdItem(fd);
    if (fileDescriptor == nullptr)
    {
        // 说明文件没有打开
        std::cerr << "Error: read---File not opened." << std::endl;
        return false;
    }
    // 判断是否为读取模式以及根据对应INode的权限判断是否有读取权限
    if ((fileDescriptor->mode & MODE_R) == 0)
    {
        // 非读取模式
        std::cerr << "Error: read---This file is not opened in read mode." << std::endl;
        return false;
    }
    // 拿到inode
    INode* iNode = fileDescriptor->iNode;
    // 判断读取权限
    int accessPermission = calculatePermission(*iNode);
    if (((iNode->iNodeMode >> accessPermission >> 2) & 1) == 0)
    {
        std::cerr << "Error: read---Permission denied." << std::endl;
        return false;
    }
    // 读取文件
    if (!sysReadFile(*iNode, fileDescriptor->offset, size, content))
    {
        std::cerr << "Error: read---Failed to read file." << std::endl;
        return false;
    }
    // 更新文件描述符的偏移量
    fileManager.setFdOffset(fd, fileDescriptor->offset + size);
    return true;
}

/**
 * 解析read命令，读取文件
 * @param arg read命令参数
 */
void FileSystem::readFile(const std::string& arg)
{
    // read fd size
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.size() != 2)
    {
        std::cerr << "Error: read---Invalid argument." << std::endl;
        std::cerr << "Usage: read <fd> <size>" << std::endl;
        return;
    }
    // 解析文件描述符
    // 判断是否可以转换为数字
    if (args[0].find_first_not_of("0123456789") != std::string::npos || args[1].find_first_not_of("0123456789") !=
        std::string::npos)
    {
        std::cerr << "Error: read---Invalid argument." << std::endl;
        std::cerr << "Usage: read <fd> <size>" << std::endl;
        return;
    }
    int fd = std::stoi(args[0]);
    // 解析读取大小
    unsigned int size = std::stoi(args[1]);
    // 读取文件
    char* content = new char[size + 1];
    if (readWithFd(fd, content, size))
    {
        // 在最后一位加上'\0'
        content[size] = '\0';
        std::cout << "Content: " << content << std::endl;
        std::cout << "Read file successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error: read---Failed to read file." << std::endl;
    }
    delete[] content;
}

/**
 * 读取文件
 * @param arg read命令参数
 */
void FileSystem::catFile(const std::string& arg)
{
    // cat [file] [[-n] [size]] [offset]
    // file: 文件名，可为绝对路径或相对路径
    // offset: 起始位置
    // size: 读取大小
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.empty() || args.size() > 5)
    {
        std::cerr << "Error: read---Invalid argument." << std::endl;
        return;
    }
    std::string fileName = args[0];
    unsigned int offset = 0, size = 0;
    bool sized = false;
    // 其他参数 -n size -o offset 需要遍历，找到-n和-o的位置，然后解析size和offset
    for (int i = 1; i < args.size(); ++i)
    {
        if (args[i] == "-n")
        {
            if (i + 1 < args.size())
            {
                // 判断是否可以转换为数字
                if (args[i + 1].find_first_not_of("0123456789") != std::string::npos)
                {
                    std::cerr << "Error: read---Invalid argument." << std::endl;
                    return;
                }
                size = std::stoi(args[i + 1]);
                sized = true;
            }
            else
            {
                std::cerr << "Error: read---Invalid argument." << std::endl;
                return;
            }
        }
        else if (args[i] == "-o")
        {
            if (i + 1 < args.size())
            {
                offset = std::stoi(args[i + 1]);
            }
            else
            {
                std::cerr << "Error: read---Invalid argument." << std::endl;
                return;
            }
        }
    }
    // 找到文件所在的inode，首先用changeDir函数找到文件所在的上级目录
    int inodeAddr = currentDir;
    std::string dirName = currentDirName;
    std::string targetDirName = ".";
    if (fileName.find('/') != std::string::npos)
    {
        int pos = fileName.find_last_of('/');
        targetDirName = fileName.substr(0, pos);
        fileName = fileName.substr(pos + 1);
    }
    // 使用changeDir函数找到文件的inode，并切换到文件所在目录
    if (changeDir(targetDirName, inodeAddr, dirName))
    {
        // 打开文件
        int fd = openFile(dirName, inodeAddr, fileName, MODE_R);
        // 如果没有指定size，那么默认为文件大小-offset
        if (!sized)
        {
            size = fileManager.getFdItem(fd)->iNode->iNodeSize - offset;
        }
        if (fd != -1)
        {
            // 读取文件
            char* content = new char[size + 1];
            if (readWithFd(fd, content, size))
            {
                // 在最后一位加上'\0'
                content[size] = '\0';
                std::cout << "Content: " << content << std::endl;
                std::cout << "Read file successfully." << std::endl;
            }
            else
            {
                std::cerr << "Error: read---Failed to read file." << std::endl;
            }
            // 关闭文件
            closeWithFd(fd);
            delete[] content;
        }
        else
        {
            std::cerr << "Error: read---Failed to read file." << std::endl;
        }
    }
    else // 错误
    {
        std::cerr << "Error: read---Failed to find file." << std::endl;
        return;
    }
}

/**
 * 删除文件
 * @brief 目前只支持删除文件，不支持删除目录
 * @param arg rm命令参数
 */
void FileSystem::deleteFile(const std::string& arg)
{
    // rm [file]
    // file: 文件名，可为绝对路径或相对路径
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.empty() || args.size() > 1)
    {
        std::cerr << "Error: rm---Invalid argument." << std::endl;
        return;
    }
    std::string fileName = args[0];
    // 找到文件所在的inode，首先用changeDir函数找到文件所在的上级目录
    int inodeAddr = currentDir;
    std::string dirName = currentDirName;
    std::string targetDirName = ".";
    if (fileName.find('/') != std::string::npos)
    {
        int pos = fileName.find_last_of('/');
        targetDirName = fileName.substr(0, pos);
        fileName = fileName.substr(pos + 1);
    }

    // 使用changeDir函数找到文件的inode，并切换到文件所在目录
    if (changeDir(targetDirName, inodeAddr, dirName))
    {
        // 找到之后依次查询目录inode的目录项，找到文件inode
        INode iNode = {};
        fr.seekg(inodeAddr, std::ios::beg);
        fr.read(reinterpret_cast<char*>(&iNode), sizeof(INode));
        DirItem dirItems[16] = {};
        int cnt = iNode.iNodeLink;
        // 读取目录项，此处仅判断直接块指针
        for (int i = 0; i < cnt && i < 144;)
        {
            int dirBlockNo = i / 16;
            // 如果直接块指针为空，那么直接跳过
            if (iNode.iNodeBlockPointer[dirBlockNo] == -1)
            {
                i += 16;
                continue;
            }
            // 读取目录项
            int dirItemBlockAddr = iNode.iNodeBlockPointer[dirBlockNo];
            fr.seekg(dirItemBlockAddr, std::ios::beg);
            fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
            // 遍历目录项
            for (int j = 0; j < 16; j++)
            {
                // 如果目录项名称为空，那么直接跳过
                if (dirItems[j].itemName[0] == '\0')
                {
                    j++;
                    continue;
                }
                // 如果目录项名称相同
                if (strcmp(dirItems[j].itemName, fileName.c_str()) == 0)
                {
                    // 读取inode
                    INode fileINode = {};
                    fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                    fr.read(reinterpret_cast<char*>(&fileINode), sizeof(INode));
                    // 计算权限
                    int accessPermission = calculatePermission(fileINode);
                    // 判断读取权限
                    if (((fileINode.iNodeMode >> accessPermission >> 2) & 1) == 0)
                    {
                        std::cerr << "Error: rm---Permission denied." << std::endl;
                        return;
                    }
                    // 如果是文件
                    if (!(fileINode.iNodeMode & DIR_TYPE))
                    {
                        // 释放inode
                        freeINode(dirItems[j].iNodeAddr);
                        // 释放磁盘块
                        clearINodeBlocks(fileINode);
                        // 清空目录项
                        dirItems[j].itemName[0] = '\0';
                        dirItems[j].iNodeAddr = -1;
                        // 写回目录项，索引到inode的目录项
                        fw.seekp(dirItemBlockAddr, std::ios::beg);
                        fw.write(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
                        fw.flush();
                        // 更新链接数
                        iNode.iNodeLink--;
                        // 写回inode
                        fw.seekp(inodeAddr, std::ios::beg);
                        fw.write(reinterpret_cast<char*>(&iNode), sizeof(INode));
                        // 清空缓冲区
                        fw.flush();
                        // 设置缓存一致性为false
                        iNodeCacheBitmap[iNode.iNodeNo] = false;
                        std::cout << "File deleted successfully." << std::endl;
                        return;
                    }
                }
            }
        }
        // 错误
        std::cerr << "Error: rm---Failed to find file." << std::endl;
        std::cerr.flush();
        return;
    }
    else // 错误
    {
        std::cerr << "Error: rm---Failed to find file." << std::endl;
        std::cerr.flush();
        return;
    }
}

/**
 * 写入文件
 * @brief write [file] [content] [offset]
 * @param arg write命令参数
 */
void FileSystem::echoFile(const std::string& arg)
{
    // echo content > file
    // file: 文件名，可为绝对路径或相对路径
    // content: 写入内容,content中可以包含空格
    // 按照最后一个空格分割参数，前面'>'前为content，后面为file，content还需要去掉最后一个空格
    int pos = arg.find_last_of(' ');
    // 如果没有找到空格或者空格位置小于3或者前一个字符不为'>'
    if (pos == std::string::npos || pos <= 2 || arg[pos - 1] != '>')
    {
        std::cerr << "Error: echo---Invalid argument." << std::endl;
        std::cerr << "Usage: echo content > file" << std::endl;
        return;
    }
    // 获取content和file
    std::string content = arg.substr(0, pos - 1);
    std::string fileName = arg.substr(pos + 1);

    // 找到文件所在的inode，首先用changeDir函数找到文件所在的上级目录
    int inodeAddr = currentDir;
    std::string dirName = currentDirName;
    std::string targetDirName = ".";
    if (fileName.find('/') != std::string::npos)
    {
        int pos = fileName.find_last_of('/');
        targetDirName = fileName.substr(0, pos);
        fileName = fileName.substr(pos + 1);
    }

    // 使用changeDir函数找到文件的inode，并切换到文件所在目录
    if (changeDir(targetDirName, inodeAddr, dirName))
    {
        // 首先打开文件
        int fd = openFile(dirName, inodeAddr, fileName, MODE_W);
        if (fd != -1)
        {
            // 写入文件
            if (writeWithFd(fd, content.c_str(), content.size()))
            {
                std::cout << "Write file successfully." << std::endl;
            }
            else
            {
                std::cerr << "Error: write---Failed to write file." << std::endl;
            }
            // 关闭文件
            closeWithFd(fd);
            return;
        }
        else
        {
            std::cerr << "Error: write---Failed to write file." << std::endl;
            return;
        }
    }
    // 错误
    std::cerr << "Error: write---Failed to find file." << std::endl;
    std::cerr.flush();
    return;
}

/**
 * vi编辑器
 * @param str
 */
void FileSystem::viEditor(char* buffer, int iNodeAddr, unsigned int size)
{
#ifdef WIN
    // 首先清空屏幕
    system("cls");
    // 设置两个光标，一个用于显示，一个用于编辑
    COORD displayCur{}, editCur{};
    // 定义一个坐标
    COORD pos = {0, 0};
    // 获取标准输出句柄
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);


#else
    // 报错，暂不支持
    std::cerr << "Error: vi---Not supported." << std::endl;
    std::cerr.flush();
    return;
#endif
}

/**
 * vi编辑文件
 * @param arg
 */
void FileSystem::vi(const std::string& arg)
{
    // vi [file]
    // file: 文件名，可为绝对路径或相对路径

    /*-------------------解析参数-------------------*/
    // 首先按照空格分割参数
    std::istringstream iss(arg);
    std::string temp;
    std::vector<std::string> args;
    while (iss >> temp)
    {
        args.push_back(temp);
    }
    // 如果参数为空或者超出范围
    if (args.empty() || args.size() > 1)
    {
        std::cerr << "Error: vi---Invalid argument." << std::endl;
        std::cerr << "Usage: vi [file]" << std::endl;
        return;
    }
    std::string fileName = args[0];
    // 将filename拆分为文件名和目录名
    std::string dirName = ".";
    if (fileName.find('/') != std::string::npos)
    {
        int pos = fileName.find_last_of('/');
        dirName = fileName.substr(0, pos);
        fileName = fileName.substr(pos + 1);
    }

    /*-------------------找到文件所在的inode并读取文件-------------------*/

    // 找到文件所在的inode，首先用changeDir函数找到文件所在的上级目录
    int inodeAddr = currentDir;

    // 使用changeDir函数找到文件的inode，并切换到文件所在目录
    if (changeDir(dirName, inodeAddr, dirName))
    {
        // 找到之后依次查询目录inode的目录项，找到文件inode
        INode iNode = {};
        fr.seekg(inodeAddr, std::ios::beg);
        fr.read(reinterpret_cast<char*>(&iNode), sizeof(INode));
        DirItem dirItems[16] = {};
        int cnt = iNode.iNodeLink;
        // 读取目录项，此处仅判断直接块指针
        for (int i = 0; i < cnt && i < 144;)
        {
            int dirBlockNo = i / 16;
            // 如果直接块指针为空，那么直接跳过
            if (iNode.iNodeBlockPointer[dirBlockNo] == -1)
            {
                i += 16;
                continue;
            }
            // 读取目录项
            int dirItemBlockAddr = iNode.iNodeBlockPointer[dirBlockNo];
            fr.seekg(dirItemBlockAddr, std::ios::beg);
            fr.read(reinterpret_cast<char*>(&dirItems), sizeof(dirItems));
            // 遍历目录项
            for (int j = 0; j < 16; j++)
            {
                // 如果目录项名称为空，那么直接跳过
                if (dirItems[j].itemName[0] == '\0')
                {
                    j++;
                    continue;
                }
                // 如果目录项名称相同
                if (strcmp(dirItems[j].itemName, fileName.c_str()) == 0)
                {
                    // 读取inode
                    INode fileINode = {};
                    fr.seekg(dirItems[j].iNodeAddr, std::ios::beg);
                    fr.read(reinterpret_cast<char*>(&fileINode), sizeof(INode));
                    // 计算权限
                    int accessPermission = calculatePermission(fileINode);
                    // 判断读取权限
                    if (((fileINode.iNodeMode >> accessPermission >> 2) & 1) == 0)
                    {
                        std::cerr << "Error: vi---Permission denied." << std::endl;
                        return;
                    }
                    // 如果是文件
                    if (!(fileINode.iNodeMode & DIR_TYPE))
                    {
                        // 读取文件
                        // char* content = new char[maxFileSize];
                        // 声明一个9*512字节的文件内容缓冲区，因为vi编辑器最多只能编辑9个直接块
                        char* content = new char[9 * 512];
                        if (sysReadFile(fileINode, 0, fileINode.iNodeSize, content))
                        {
                            // 设置字符串结束符
                            content[fileINode.iNodeSize] = '\0';
                            // 调用vi编辑器，后续的编辑操作在vi编辑器中完成，所以还需要给定inode地址，便于后续写回
                            viEditor(content, dirItems[j].iNodeAddr, fileINode.iNodeSize);
                        }
                        else
                        {
                            std::cerr << "Error: vi---Failed to read file." << std::endl;
                        }
                        delete[] content;
                        return;
                    }
                }
            }
        }
        // 如果文件不存在，那么需要创建
    }
    // 错误
    std::cerr << "Error: vi---Failed to find file." << std::endl;
    std::cerr.flush();
    return;
}
