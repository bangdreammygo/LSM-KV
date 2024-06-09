#include "SSTable.h"


//从sstable中筛取坐标
Position SSTableCache::get(key_type k){
    //准备好二分查找的准备工作
    int right=indexes.size()-1;
    int left=0;
    bool flag=false;
    Position pos=Position();
    //执行二分查找
    while(left<=right){
        int mid=left + ((right - left) / 2);
        //查找到了
        if(indexes[mid].Key==k){
            pos.IsExist=true;
            pos.Offset=indexes[mid].Offset;
            pos.Vlen=indexes[mid].Vlen;
            flag=true;
            break;
        }
        //没查找到
        if(indexes[mid].Key>k)right=mid-1;
        else if(indexes[mid].Key<k)left=mid+1;
    }
    //查找成功
    if(flag)return pos;
    //没有查找到
    pos.IsExist=false;
    return pos;
}



//sstable是用于compact的
//从sscache构造sstable
SSTable::SSTable(SSTableCache *cache)
{
    path = cache->path;
    //读取时间戳
    timeStamp = (cache->header).timeStamp;
    //保存原sstable的键值个数
    length = (cache->header).size;
    // copy下来数据
    for(auto it = (cache->indexes).begin();it!=(cache->indexes).end();it++) {
        //读取key
        key_type key = (*it).Key;
        //读取偏置
        offset_type offset = (*it).Offset;
        //读取vlen
        vlen_type vlen=(*it).Vlen;
        //将值push回去
        this->indexes.push_back(Index(key,offset,vlen));
    }
    delete cache;
}


//归并函数
void SSTable::merge(std::vector<SSTable> &tables)
{
    uint32_t size = tables.size();
    //只有一个，归个锤子，返回去开始切割了得
    if(size == 1)
        return;
    uint32_t group = size/2;
    std::vector<SSTable> next;
    for(uint32_t i = 0; i < group; ++i) {
        //两两merge
        next.push_back(merge2(tables[2*i], tables[2*i + 1]));
    }
    //奇数个
    if(size % 2 == 1)
        next.push_back(tables[size - 1]);
    merge(next);
    //table变成一个很长很长的大table等待切割
    tables = next;
}

///////////////////合并的结果
SSTable SSTable::merge2(SSTable &a, SSTable &b)
{
    //记录结果
    SSTable result;
    //记录时间戳
    result.timeStamp = a.timeStamp;
    //归并排序两个sstable的key
    //从小到大
    //两边都非空
    while((!a.indexes.empty())&&(!b.indexes.empty())){
        //获取链表头部的key
        key_type aKey = a.indexes.front().Key;
        key_type bKey = b.indexes.front().Key;
        if(aKey > bKey) {
            result.indexes.push_back(b.indexes.front());
            b.indexes.pop_front();
        }
        else if(aKey < bKey) {
            result.indexes.push_back(a.indexes.front());
            a.indexes.pop_front();
        }
        //当两个key重合时，舍去时间戳较小的那个
        else{
            result.indexes.push_back(a.indexes.front());
            a.indexes.pop_front();
            b.indexes.pop_front();
        }
    }
    //归并剩下的没有结束的序列
    while(!a.indexes.empty()){
        result.indexes.push_back(a.indexes.front());
        a.indexes.pop_front();
    }
    while(!b.indexes.empty()){
        result.indexes.push_back(b.indexes.front());
        b.indexes.pop_front();
    }
    return result;
}

////////////////////向sstable中新增
void SSTable::add(const Index &index)
{
    size += 20;
    length++;
    indexes.push_back(index);
}

///////////////////切割sstable
std::vector<SSTableCache*> SSTable::save(const std::string &dir)
{
    std::vector<SSTableCache*> caches;
    SSTable newTable;
    //记录写了几个
    uint64_t num = 0;
    while(!indexes.empty()) {
        //看看是否超长度了
        if(newTable.size+20>= MAX_TABLESIZE) {
            caches.push_back(newTable.saveSingle(dir, timeStamp, num++));
            newTable = SSTable();
        }
        //加入新的一项
        newTable.add(indexes.front());
        indexes.pop_front();
    }
    //全部写入了，要把new里剩下的也写进去
    if(newTable.length > 0) {
        caches.push_back(newTable.saveSingle(dir, timeStamp, num));
    }
    return caches;
}
///////////////////写入单个sstable
SSTableCache *SSTable::saveSingle(const std::string &dir, const uint64_t &currentTime, const uint64_t &num)
{
    SSTableCache *cache = new SSTableCache;

    char *buffer = new char[size];
    BloomFilter *filter = cache->bloomFilter;
    //写时间戳
    *(uint64_t*)buffer = currentTime;
    (cache->header).timeStamp = currentTime;
    //写长度
    *(uint64_t*)(buffer + 8) = length;
    (cache->header).size = length;
    //写最小值
    *(uint64_t*)(buffer + 16) = indexes.front().Key;
    (cache->header).minKey = indexes.front().Key;
    //留出布隆过滤器的空闲
    char *index = buffer +32+FILTER_SIZE/8;
    for(auto it = indexes.begin(); it != indexes.end(); ++it) {
        filter->add((*it).Key);
        //写键值
        *(uint64_t*)index = (*it).Key;
        index += 8;
        //写偏置
        *(uint64_t*)index = (*it).Offset;
        index += 8;
        //写长度
        *(uint32_t*)index = (*it).Vlen;
        index += 4;
        //插入indexes里
        (cache->indexes).push_back(Index((*it).Key,(*it).Offset,(*it).Vlen));
    }
    //写最大键值
    *(uint64_t*)(buffer + 24) = indexes.back().Key;
    (cache->header).maxKey = indexes.back().Key;
    //写过滤器
    filter->save2Buffer(buffer + 32);

    std::string filename = dir + "/" + std::to_string(currentTime) + "-" + std::to_string(num) + ".sst";
    cache->path = filename;
    std::ofstream outFile(filename, std::ios::binary | std::ios::out);
    outFile.write(buffer, size);

    delete[] buffer;
    outFile.close();
    return cache;
}




////////////////////////////////////////////////辅助函数区域//////////////////////

//比较两个cache的时间戳大小
bool cacheTimeCompare(SSTableCache *a, SSTableCache *b)
{
    return (a->header).timeStamp > (b->header).timeStamp;
}
//比较两个sstable的时间戳大小
bool tableTimeCompare(SSTable &a, SSTable &b)
{
    return a.timeStamp > b.timeStamp;
}
//检测有没有相交的区域
bool haveIntersection(const SSTableCache *cache, const std::vector<Range> &ranges)
{
    //获取最大最小值
    uint64_t min = (cache->header).minKey, max = (cache->header).maxKey;
    //与一众range相交检查
    for(auto it = ranges.begin(); it != ranges.end(); ++it) {
        if(!(((*it).max < min) || ((*it).min > max))) {
            return true;
        }
    }
    return false;
}



////////////最后用来持久化的构造函数
SSTableCache::SSTableCache(const std::string &dir)
{
    path = dir;
    std::ifstream file(dir, std::ios::binary);
    if(!file) {
        printf("Fail to open file %s", dir.c_str());
        exit(-1);
    }
    //读取header
    file.read((char*)&header.timeStamp, 8);
    file.read((char*)&header.size, 8);
    uint64_t length = header.size;
    file.read((char*)&header.minKey, 8);
    file.read((char*)&header.maxKey, 8);

    // 加载布隆过滤器
    char *filterBuf = new char[FILTER_SIZE/8];
    file.read(filterBuf, FILTER_SIZE/8);
    bloomFilter = new BloomFilter(filterBuf);

    //加载sstable
    char *indexBuf = new char[length * 20];
    file.read(indexBuf, length * 20);
    for(uint32_t i = 0; i < length; ++i) {
        indexes.push_back(Index(*(uint64_t*)(indexBuf + 20*i), *(uint64_t*)(indexBuf + 20*i + 8),*(uint32_t*)(indexBuf + 20*i + 16)));
    }
    delete[] filterBuf;
    delete[] indexBuf;
    file.close();

}