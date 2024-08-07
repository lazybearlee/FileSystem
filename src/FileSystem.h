//
// Created by 李卓 on 24-6-9.
//
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <bitset>
#include <FSHelper.h>
#include <VDisk.h>
#include <INodeCache.h>

#include "FileManager.h"
// 如果是Windows系统
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define WIN
#else
// 如果是Linux系统
#include <unistd.h>
#endif

/* 命令 */
#define LS_DEFAULT 0
#define LS_L 1


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
    void printBuffer();
    void readSuperBlock();
    bool installed;

    /*-------------------文件系统基本操作-------------------*/

    void getHostName();
    int allocateINode();
    bool freeINode(int iNodeAddr);
    int allocateBlock();
    bool freeBlock(int blockAddr);
    bool formatFileSystem();
    void initSystemConfig();
    void createFileSystem(const std::string& systemName);
    bool installFileSystem();
    void uninstall();
    void load(const std::string &filename);
    void save();
    void executeInFS(const std::string &command);
    void printINode(const INode& node);
    void printSuperBlock();
    void printSystemInfo();
    void printUserAndHostName();
    void clearScreen();
    std::string getAbsolutePath(const std::string& dirName, const std::string& INodeName);
    INode* findINodeInDir(INode& dirINode, const std::string& dirName, const std::string& INodeName, int type);
    FreeDirItemIndex findFreeDirItem(const INode& dirINode, const std::string& itemName, int itemType);
    FreeDirItemIndex findFreeDirItem(const INode& dirINode);
    void clearINodeBlocks(INode& iNode);
    int calculatePermission(const INode& iNode);
    INode* findINodeInCache(const std::string& absolutePath, int type);

    /*-------------------文件系统文件操作-------------------*/

    int openFile(std::string& dirName, int dirAddr, std::string& fileName, int mode);
    void openWithFilename(const std::string& arg);
    bool closeWithFd(int fd);
    void closeFile(const std::string& arg);
    void seekWithFd(const std::string& arg);
    bool createFileHelper(const std::string& fileName, int dirINodeAddr, char* content, unsigned int size);
    void createFile(const std::string& arg);
    bool sysReadFile(INode& iNode, unsigned int offset, unsigned int size, char* content);
    bool readWithFd(int fd, char* content, unsigned int size);
    void readFile(const std::string& arg);
    void catFile(const std::string& arg);
    bool sysWriteFile(INode& iNode, const char* content, unsigned int size, unsigned int offset);
    bool writeWithFd(int fd, const char* content, unsigned int size);
    void writeFile(const std::string& arg);
    void deleteFile(const std::string& arg);
    void echoFile(const std::string& arg);

    /*-------------------文件系统目录操作-------------------*/

    void makeDir(const std::string& arg);
    bool mkdirHelper(bool pFlag, const std::string& dirName, int inodeAddr);
    bool rmdirHelper(bool ignore, bool pFlag, const std::string& dirName, int inodeAddr);
    void removeDir(const std::string& arg);
    void listDir(const std::string& arg);
    void listDirByINode(int inodeAddr, int lsMode);
    bool changeDir(const std::string& arg, int& currentDir, std::string& currentDirName);
    void printCurrentDir();

    /*-------------------文件系统用户操作-------------------*/

    void login(const std::string &userName, const std::string &password);
    void logout();
    void createUser(const std::string &userName, const std::string &password);
    void deleteUser(const std::string &userName);
    void changePassword(const std::string &userName, const std::string &password);
    void listUser();
    void listGroup();

    /*-------------------文件系统权限操作-------------------*/

    void changeMode(const std::string &filename, const std::string &mode);
    void changeOwner(const std::string &filename, const std::string &owner);
    void changeGroup(const std::string &filename, const std::string &group);

    /*-------------------文件系统其他操作-------------------*/

    void viEditor(char* str, int inodeAddr, unsigned int size);
    void vi(const std::string& arg);

private:
    /*-------------------系统-------------------*/
    // 读系统文件的指针
    std::ifstream fr;
    // 写系统文件的指针
    std::ofstream fw;
    // 超级块
    SuperBlock superBlock{};
    // inode位图
    std::bitset<MAX_INODE_NUM> iNodeBitmap;
    // 数据块位图
    std::bitset<BLOCK_NUM> blockBitmap;
    // 判断inode缓存是否与磁盘同步的位图
    std::bitset<MAX_INODE_NUM> iNodeCacheBitmap;
    // FileManager类
    FileManager fileManager;
    // // 文件inode缓存
    // INodeCache fileINodeCache;
    // 目录inode缓存
    INodeCache dirINodeCache;

    /*-------------------用户-------------------*/
    // 用户状态
    UserState userState{};
    // 当前目录
    int currentDir{};
    // 当前用户主目录
    int userHomeDir;
    // 当前目录名
    std::string currentDirName;
    // 当前主机名
    char hostName[MAX_HOST_NAME_LEN]{};
    // 根目录inode节点
    int rootINodeAddr{};
    // 下一个要被分配的用户标识
    int nextUserID;
    // 下一个要被分配的组标识
    int nextGroupID;

};

#endif // FILESYSTEM_H

