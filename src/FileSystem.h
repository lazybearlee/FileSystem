//
// Created by 李卓 on 24-6-9.
//
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <bitset>
#include <FSHelper.h>
#include <fstream>
#include <string>
// 如果是Windows系统
#ifdef _WIN32
#include <windows.h>
#else
// 如果是Linux系统
#include <unistd.h>
#endif


/* 定义常量 */
// 版本号
#define VERSION "1.0"
// 作者
#define AUTHOR "Li Zhuo"
// 邮箱
#define EMAIL "Lz1958455046@outlook.com or 1120211231@bit.edu.cn"
// 数据块大小
#define BLOCK_SIZE 512
// inode节点大小
#define INODE_SIZE 128
// 数据块指针数目
#define BLOCK_POINTER_NUM 12
// 目录项名称最大长度
#define MAX_DIRITEM_NAME_LEN 28
// 虚拟磁盘大小，即32MB
#define MAX_DISK_SIZE (3200*1000)
// BLOCK数目，暂定为1024
#define BLOCK_NUM 1024
// inode节点数目，暂定为512
#define MAX_INODE_NUM 512
// 每个块组的块数
#define BLOCK_GROUP_SIZE 32
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

/* 用户 */
// 用户名最大长度
#define MAX_USER_NAME_LEN 20
// 用户组名最大长度
#define MAX_GROUP_NAME_LEN 20
// 用户密码最大长度
#define MAX_PASSWORD_LEN 20
// 用户状态
#define MAX_USER_STATE 1
// 主机名最大长度
#define MAX_HOST_NAME_LEN 32

/* 命令 */
#define LS_DEFAULT 0
#define LS_L 1

/* 全局变量 */
// 超级块开始位置,占一个磁盘块
const int superBlockStartPos = 0;
// inode位图开始位置,位图占两个磁盘块
const int iNodeBitmapStartPos = 1 * BLOCK_SIZE;
// 数据块位图开始位置，数据块位图占20个磁盘块
const int blockBitmapStartPos = iNodeBitmapStartPos + 2 * BLOCK_SIZE;
// inode节点开始位置,占 INODE_NUM/(BLOCK_SIZE/INODE_SIZE) 个磁盘块即 129 个磁盘块
const int iNodeStartPos = blockBitmapStartPos + 20 * BLOCK_SIZE;
// 数据块开始位置，占 BLOCK_NUM 个磁盘块
const int blockStartPos = iNodeStartPos + (MAX_INODE_NUM * INODE_SIZE) / BLOCK_SIZE + 1;
// 虚拟磁盘文件大小
const int diskBlockSize = blockStartPos + BLOCK_NUM * BLOCK_SIZE;
// 单个虚拟磁盘文件文件最大大小
const int maxFileSize = 9 * BLOCK_SIZE + BLOCK_SIZE/sizeof(int) * BLOCK_SIZE + BLOCK_SIZE/sizeof(int) * BLOCK_SIZE * BLOCK_SIZE/sizeof(int);

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
    int freeBlockStack[BLOCK_GROUP_SIZE];
    // 堆栈顶指针
    int freeBlockAddr;

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
    char iNodeOwner[MAX_USER_NAME_LEN];
    // 文件所属组，字符串
    char iNodeGroup[MAX_GROUP_NAME_LEN];
    // 文件大小
    unsigned int iNodeSize;
    // 文件创建时间，时间戳
    time_t iNodeCreateTime;
    // 文件修改时间，时间戳
    time_t iNodeModifyTime;
    // 文件访问时间，时间戳
    time_t iNodeAccessTime;
    // 文件数据块指针, 9个直接指针，1个一级间接指针，1个二级间接指针，1个三级间接指针
    int iNodeBlockPointer[BLOCK_POINTER_NUM];
};

/**
 * 目录项
 */
struct DirItem
{
    // 目录项名
    char itemName[MAX_DIRITEM_NAME_LEN];
    // inode地址
    int iNodeAddr;
};

/**
 * 用户状态
 */
struct UserState
{
    // 是否已经登录
    bool isLogin;
    // 用户名
    char userName[MAX_USER_NAME_LEN];
    // 用户所在组
    char userGroup[MAX_GROUP_NAME_LEN];
    // 用户密码
    char userPassword[MAX_PASSWORD_LEN];
    // 用户ID
    unsigned short userID;
    // 用户组ID
    unsigned short userGroupID;
};

class FileSystem {
public:
    // 构造函数
    FileSystem();
    // 析构函数
    ~FileSystem();
    bool installed;
    /*-------------------文件系统基本操作-------------------*/
    void getHostName();
    int allocateINode();
    bool freeINode(int iNodeAddr);
    int allocateBlock();
    bool freeBlock(int blockAddr);
    bool formatFileSystem();
    void initSystemConfig();
    void create(const std::string &systemName);
    bool installFileSystem();
    void uninstall();
    void load(const std::string &filename);
    void save();
    void help(const std::string& command);
    void executeInFS(const std::string &command);
    void printSuperBlock();
    void printSystemInfo();
    /*-------------------文件系统文件操作-------------------*/
    void createFile(std::string args[2]);
    void readFile(std::string args[2]);
    void writeFile(std::string args[2]);
    void deleteFile(std::string args[2]);
    /*-------------------文件系统目录操作-------------------*/
    // 创建目录，对应mkdir命令
    void makeDir(const std::string& arg);
    bool mkdirHelper(bool pFlag, const std::string& dirName, int inodeAddr);
    // 删除目录，对应rmdir命令
    void removeDir(const std::string& arg);
    // 列出当前目录下的文件和目录，对应ls命令
    void listDir(const std::string& arg);
    void listDirByINode(int inodeAddr, int lsMode);
    // 切换目录
    bool changeDir(const std::string& arg);
    // 打印当前目录，对应pwd命令
    void printCurrentDir();
    /*-------------------文件系统用户操作-------------------*/
    // 即用户登录，可以用于切换用户，对应su命令
    void login(const std::string &userName, const std::string &password);
    void logout();
    // 创建用户，对应useradd命令
    void createUser(const std::string &userName, const std::string &password);
    // 删除用户，对应userdel命令
    void deleteUser(const std::string &userName);
    // 修改用户密码，对应passwd命令
    void changePassword(const std::string &userName, const std::string &password);
    // 列出用户，对应who命令
    void listUser();
    // 列出用户组，对应group命令
    void listGroup();
    /*-------------------文件系统权限操作-------------------*/
    void changeMode(const std::string &filename, const std::string &mode);
    void changeOwner(const std::string &filename, const std::string &owner);
    void changeGroup(const std::string &filename, const std::string &group);
    /*-------------------文件系统其他操作-------------------*/
    void help();
    // 清空屏幕
    void clear();

private:
    /*-------------------系统-------------------*/
    // 虚拟磁盘缓冲区，初始为最大缓冲区大小
    static char diskBuffer[MAX_DISK_SIZE];
    // 读系统文件的指针
    std::ifstream fr;
    // 写系统文件的指针
    std::ofstream fw;
    // 替换为读写系统文件的指针
    std::fstream frw;
    // 超级块
    SuperBlock superBlock;
    // inode位图
    std::bitset<MAX_INODE_NUM> iNodeBitmap;
    // 数据块位图
    std::bitset<BLOCK_NUM> blockBitmap;

    /*-------------------用户-------------------*/
    // 用户状态
    UserState userState;
    // 当前目录
    int currentDir;
    // 当前用户主目录
    int userHomeDir;
    // 当前目录名
    std::string currentDirName;
    // 当前主机名
    char hostName[MAX_HOST_NAME_LEN];
    // 根目录inode节点
    int rootINodeAddr;
    // 下一个要被分配的用户标识
    int nextUserID;
    // 下一个要被分配的组标识
    int nextGroupID;

};

#endif // FILESYSTEM_H

