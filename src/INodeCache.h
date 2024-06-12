//
// Created by 李卓 on 24-6-12.
//

#ifndef INODECACHE_H
#define INODECACHE_H

#include <VDisk.h>


/**
 * 被缓存的INode节点
 * 是一个双向链表节点
 */
struct INodeCacheItem
{
    // file name， key
    std::string fileName;
    // inode节点地址， value
    int iNodeAddr;
    // inode节点， value
    INode iNode;
    // 前一个节点
    INodeCacheItem* prev;
    // 后一个节点
    INodeCacheItem* next;
};

/**
 * INode缓存
 * 用于管理文件系统中的被缓存的INode节点，采用LRU算法
 */
class INodeCache
{
public:
    // INode缓存，使用哈希表存储
    std::map<std::string, INodeCacheItem*> cache;
    // 文件描述符的位图，用于查找空闲的文件描述符
    std::bitset<MAX_INODE_NUM> inodeBitmap;

    // 缓存大小
    int capacity;
    // 当前缓存大小
    int currentSize;

    // 头指针
    INodeCacheItem* head;
    // 尾指针
    INodeCacheItem* tail;

    explicit INodeCache(int capacity);
    INodeCacheItem* findINodeCacheItem(const std::string& fileName);

    // 通过文件名查找INode节点
    INode* findINodeByFileName(const std::string& fileName);
    int findINodeAddrByFileName(const std::string& fileName);

    // 添加INode节点到缓存
    bool addINodeToCache(const std::string& fileName, int iNodeAddr, INode iNode);

    // 从缓存中删除INode节点
    bool removeINodeFromCache(const std::string& fileName);
    void addCacheItemToHead(INodeCacheItem* item);
    void moveToHead(INodeCacheItem* item);
    void removeCacheItem(INodeCacheItem* item);
    void removeTail();
};

#endif //INODECACHE_H
