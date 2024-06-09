#include "BloomFilter.h"



//有参构造函数
BloomFilter::BloomFilter(char *buf)
{
    //将文件读入
    memcpy((char*)&BitSet, buf, FILTER_SIZE/8);
}

//向过滤器中新增数据
void BloomFilter::add(const key_type &key)
{
    uint32_t hashVal[4] = {0};
    MurmurHash3_x64_128(&key, sizeof(key), 1, &hashVal);
    BitSet.set(hashVal[0] % FILTER_SIZE);
    BitSet.set(hashVal[1] % FILTER_SIZE);
    BitSet.set(hashVal[2] % FILTER_SIZE);
    BitSet.set(hashVal[3] % FILTER_SIZE);
}
/////////////查看是否包含
bool BloomFilter::contains(const key_type &key)
{
    uint32_t hashVal[4] = {0};
    MurmurHash3_x64_128(&key, sizeof(key), 1, &hashVal);
    //检测在四个位置是否都有
    return    (BitSet[hashVal[0] % FILTER_SIZE]
            && BitSet[hashVal[1] % FILTER_SIZE]
            && BitSet[hashVal[2] % FILTER_SIZE]
            && BitSet[hashVal[3] % FILTER_SIZE]);
}
/////保存为字符串格式
void BloomFilter::save2Buffer(char *buf)
{
   memcpy(buf, (char*)&BitSet, FILTER_SIZE/8);
}
