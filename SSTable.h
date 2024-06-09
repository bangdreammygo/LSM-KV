#ifndef SSTABLE_H
#define SSTABLE_H
#include "BloomFilter.h"
#include "dataStructure.h"
#include <string>
#include <vector>
#include <list>
#include <fstream>
// 定义出sstable的最大大小
#define MAX_TABLESIZE  16384



//用于内存读取的sstable
class SSTableCache
{
public:
    //表头说明
    Header header;
    //布隆过滤器
    BloomFilter *bloomFilter;
    //存储的文件的路径
    std::string path;
    //记录所有的三元组
    std::vector<Index> indexes;

    // 相关函数
    //析构函数
    ~SSTableCache(){delete bloomFilter;}
    //无参构造
    SSTableCache():bloomFilter(new BloomFilter()){}
    //从文件中读取：
    SSTableCache(const std::string &dir);
    //从sstable中获取到value(由于add是靠跳表，delete本质也是add，所以只需要行使get功能就好)
    Position get(key_type key);
    //直接筛查这个key在不在这个sstable里
    bool isKeyNotExist(key_type k){
        //超出大小区间了，过掉
        if(k<header.minKey|k>header.maxKey){
            return true;
        }
        //不在布隆过滤器中，筛掉
        if(!bloomFilter->contains(k))return true;
        //以上都筛不掉，可能存在
        return false;
    }
};




//真正的sstable
//或者说，更高级的抽象的sstable,行使更高层级的功能
class SSTable
{
public:
    //时间戳
    uint64_t timeStamp;
    std::string path;
    uint64_t size;
    uint64_t length;
    //记录所有的三元组
    std::list<Index> indexes;
    //利用一个cache生成sstable
    SSTable(SSTableCache *cache);
    //无参构造
    SSTable(): size(FILTER_SIZE/8+32), length(0) {}
    //合并多个sstable(compact的时候使用)
    static void merge(std::vector<SSTable> &tables);
    //合并2个sstable，和compact强相关
    static SSTable merge2(SSTable &a, SSTable &b);
    //写sstable
    std::vector<SSTableCache*> save(const std::string &dir);
    //将sstable写进磁盘的某个目录下
    SSTableCache *saveSingle(const std::string &dir, const uint64_t &currentTime, const uint64_t &num);
    //向sstable中增加新的一个元组
    void add(const Index &index);
};
//用于比较时间戳的函数
bool cacheTimeCompare(SSTableCache *a, SSTableCache *b);
//比较时间戳的函数
bool tableTimeCompare(SSTable &a, SSTable &b);
//是否有范围覆盖
bool haveIntersection(const SSTableCache *cache, const std::vector<Range> &ranges);

#endif