#ifndef  BLFT
#define  BLFT
///////布隆过滤器
#include "dataStructure.h"
#include <cstdint>
#include <vector>
#include <random>
#include <string>
#include <iostream>
#include <list>
#include <utility>
#include <string.h>
#include "MurmurHash3.h"
// 这个库包含的是二进制数组，支持下标访问，同时节省了很多的空间
#include <bitset>
#define FILTER_SIZE 65536

class BloomFilter
{
private:
    //8192字节长度的布隆过滤器
    std::bitset<FILTER_SIZE> BitSet;
public:
    //无参构造函数
    BloomFilter() {BitSet.reset();}
    //有参构造(从文件中读取到内存中时会需要)
    BloomFilter(char *buf);
    //新增数据
    void add(const key_type &key);
    //查看是否包含该数据
    bool contains(const key_type &key);
    //获取数据信息
    std::bitset<FILTER_SIZE> *getSet() {return &BitSet;}
    //将过滤器保存为字符串
    void save2Buffer(char* buf);
};

#endif 