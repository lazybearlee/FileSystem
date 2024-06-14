//
// Created by 李卓 on 24-6-12.
//

#include <INodeCache.h>

INodeCache::INodeCache(int capacity)
{
    // 初始化缓存大小
    this->capacity = capacity;
    // 初始化当前缓存大小
    this->currentSize = 0;
    // 初始化哈希表
    this->cache.clear();
    // 初始化头尾指针
    this->head = new INodeCacheItem();
    this->head->prev = nullptr;
    this->tail = new INodeCacheItem();
    this->tail->next = nullptr;
    this->head->next = this->tail;
    this->tail->prev = this->head;
}

/**
 * 查找INode缓存项
 * @param fileName
 * @return INode缓存项
 */
INodeCacheItem* INodeCache::findINodeCacheItem(const std::string& fileName)
{
    // 如果缓存为空
    if (this->cache.empty())
    {
        return nullptr;
    }
    // 使用哈希表查找
    auto it = this->cache.find(fileName);
    if (it != this->cache.end())
    {
        // 如果找到了，将该节点移到头部
        moveToHead(it->second);
        return it->second;
    }
    return nullptr;
}

/**
 * 查找INode节点
 * @param fileName
 * @return
 */
INode* INodeCache::findINodeByFileName(const std::string& fileName)
{
    // 查找缓存项
    INodeCacheItem* item = findINodeCacheItem(fileName);
    if (item != nullptr)
    {
        return &item->iNode;
    }
    return nullptr;
}

/**
 * 查找INode地址
 * @param fileName
 * @return INode地址
 */
int INodeCache::findINodeAddrByFileName(const std::string& fileName)
{
    // 查找缓存项
    INodeCacheItem* item = findINodeCacheItem(fileName);
    if (item != nullptr)
    {
        // 根据iNode的序号计算iNode的地址
        int iNodeAddr = iNodeStartPos + item->iNode.iNodeNo * INODE_SIZE;
        return iNodeAddr;
    }
    return -1;
}

/**
 * 添加INode节点到缓存
 * @param fileName
 * @param iNodeAddr
 * @param iNode
 */
bool INodeCache::addINodeToCache(const std::string& fileName, INode iNode)
{
    // 首先判断是否已经存在
    INodeCacheItem* item = findINodeCacheItem(fileName);
    if (item != nullptr)
    {
        item->iNode = iNode;
        // 将该节点移到头部
        moveToHead(item);
        return true;
    }
    // 如果不存在，创建新节点
    auto newItem = new INodeCacheItem();
    newItem->fileName = fileName;
    newItem->iNode = iNode;
    // 将新节点插入到头部
    addCacheItemToHead(newItem);
    // 更新哈希表
    this->cache[fileName] = newItem;
    // 更新当前缓存大小
    this->currentSize++;
    // 如果超出缓存大小，淘汰尾部节点
    if (this->currentSize > this->capacity)
    {
        removeTail();
    }
    return true;
}

/**
 * 从缓存中删除INode节点
 * @param fileName
 */
bool INodeCache::removeINodeFromCache(const std::string& fileName)
{
    // 查找缓存项
    INodeCacheItem* item = findINodeCacheItem(fileName);
    if (item != nullptr)
    {
        // 从哈希表删除
        this->cache.erase(fileName);
        // 从链表删除
        removeCacheItem(item);
        // 释放内存
        delete item;
        // 更新当前缓存大小
        this->currentSize--;
        return true;
    }
    return false;
}

/**
 * 添加缓存项到头部
 */
void INodeCache::addCacheItemToHead(INodeCacheItem* item)
{
    // 插入到头部
    item->next = this->head->next;
    item->prev = this->head;
    this->head->next->prev = item;
    this->head->next = item;
}

/**
 * 删除缓存项但不释放内存
 */
void INodeCache::removeCacheItem(INodeCacheItem* item)
{
    // 从原位置删除
    item->prev->next = item->next;
    item->next->prev = item->prev;
}

/**
 * 淘汰尾部缓存项
 */
void INodeCache::removeTail()
{
    // 获取尾部节点
    INodeCacheItem* item = this->tail->prev;
    // 从原位置删除
    removeCacheItem(item);
    // 从哈希表删除
    this->cache.erase(item->fileName);
    // 更新当前缓存大小
    this->currentSize--;
    // 释放内存
    delete item;
}

/**
 * 将缓存项移到头部
 */
void INodeCache::moveToHead(INodeCacheItem* item)
{
    // 从原位置删除
    removeCacheItem(item);
    // 插入到头部
    addCacheItemToHead(item);
}





