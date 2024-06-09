//
// Created by asus on 2024/4/6.
//
#include "skiplist.h"
//析构函数
skiplist::~skiplist() {
      node*del,*work;
      del=head;
      work=head;
      //work负责向下移动，del负责清理掉一整行
      while(del) {
        work = work->down;
        while(del) {
            node*next = del->right;
            delete del;
            del = next;
        }
        del = work;
    }
}
//获取表长
int skiplist::getLength() {
    return listLength;
}
//获取sstable的大小
key_type skiplist::getSize() {
    return listSize;
}
//插入操作,返回表中是否本来就有key
bool skiplist::put(const key_type &key, const value_type &val) {
    std::vector<node*> pathList;    //从上至下记录搜索路径
    node*p=head;
    node*tmp;
    //当p没有触底时：
    while(p){
        //p对应的key值小于目标key时，就一直向右运动
        while((tmp = p->right) && tmp->key < key) {
            p = tmp;
        }
        //目前停下来了，停下来的可能性有几种，一种是找到了一个等于key的值
        if((tmp=p->right)&&(tmp->key==key)){
            //找到了相同的key，那肯定是得替换的
            p=tmp;
            //如果本身这个值就是value，那就小丑一波无用插入，返回false即可
            if (p->val == val)return false;
            //如果这个值本身不是value，那就要重新覆盖了捏
            //注意：此时不会对表本身的长度有影响，也不会对size有影响
            //向下把值全部覆盖掉
            while (p) {
                p->val = val;
                p = p->down;
            }
            return true;
        }
        //另一情况是找到了下面一个大于key或者下一个是null的
        //那么记录路径
        pathList.push_back(p);
        p = p->down;
    }
    //退出循环了都还没返回的，说明已经找到了插入位置;目前p为空
    bool insertUp = true;
    node* downNode= nullptr;
    //向上生长
    while(insertUp && pathList.size() > 0){   //从下至上搜索路径回溯，50%概率
        node *insert = pathList.back();
        pathList.pop_back();
        insert->right = new node(insert->right, downNode, key, val); //add新结点
        downNode = insert->right;    //把新结点赋值为downNode
        insertUp = (rand()&1);   //50%概率
    }
    //退出可能是没有继续生长了，也可能是已经达到最大高度了
    //如果已经到了高度，那就继续向上生长
    if(insertUp){  //插入新的头结点，加层
        node * oldHead = head;
        head = new node();
        head->right = new node(NULL, downNode, key, val);
        head->down = oldHead;
    }
    //生长结束
    listLength++;
    updateScanptr();
    listSize+=20;//(8byte key 8byte offset 4byte vlen)
    return false;
}
//查找操作
value_type*skiplist::get(const key_type &key) {
    bool haveFound= false;
    node*cur=head;
    node*tmp;
    //从头节点开始向右下进军
    while(cur){
        //向右走
        while((tmp = cur->right) && tmp->key < key)
        {
            cur = tmp;
        }
        if((tmp = cur->right)&&tmp->key==key){
            cur=tmp;
            haveFound=true;
            break;
        }
        cur=cur->down;
    }
    //返回的是value类型的指针，不是value!;
    if(haveFound) return &(cur->val);
    else return nullptr;
}
//删除操作 返回是否被删除or是否存在
bool skiplist::remove(const key_type &key) {
    bool isExist= put(key,"~DELETED~");
    return  isExist;
}
//打印一下删除的返回信息
void skiplist::readRemoveresult(bool flag) {
    if(flag)std::cout<<"we have delete it\n";
    else std::cout<<"this value do not exist in list\n";
}
//打印查找结果，debug用
void skiplist::readGetResult(value_type*val) {
   if(val&&(*val!="~DELETE~")) std::cout<<"we have found "<<*val<<std::endl;
   else if(*val=="~DELETE~") std::cout<<"this key has been deleted\n";
   else std::cout<<"your key do not exist\n";
}

//scan功能
std::list<std::pair<key_type,value_type>> skiplist::scan(key_type k1,key_type k2){
    //创建返回值
    std::list<std::pair<key_type,value_type>>res;
    //scan指针始终位于跳表最底部
    //一直向后移动
    while(scanptr->right&&scanptr->key<k1)scanptr=scanptr->right;
    //查看是否是到头了
    while(scanptr->right&&scanptr->right->key<=k2){
        scanptr=scanptr->right;
        res.push_back(std::pair<key_type,value_type>(scanptr->key,scanptr->val));
    }
    updateScanptr();
    return res;
}
//保持scan指针在链表底部
void skiplist:: updateScanptr(){
    scanptr=head;
    while(scanptr->down!=nullptr)scanptr=scanptr->down;
}

//将跳表写入sstable
SSTableCache* skiplist::save2SSTable(const std::string &dir, const uint64_t &currentTime , vlogType &vlog){
    //到时候返回去的值
    SSTableCache *cache = new SSTableCache;
    //到时候要写的entry表
    std::vector<Entry>entries;
    //获取目前跳表的最小值
    node*cur=getListHead();
    //获取布隆过滤器
    BloomFilter*blfter=cache->bloomFilter;
    //获取文件名
    std::string filename=dir+"/"+std::to_string(currentTime)+".sst";
    //先把存储所需要的字符串准备出来
    char*buffer=new char[listSize];
    //读取时间戳，并同时修改sstablecache里的时间戳
    *(uint64_t*)buffer = currentTime;
    (cache->header).timeStamp = currentTime;
    //存储键值的数目
    *(uint64_t*)(buffer + 8) = listLength;
    (cache->header).size = listLength;
    //存储最小值
    *(uint64_t*)(buffer + 16) = cur->key;
    (cache->header).minKey = cur->key;
    //最大值需要等到最后全部遍历结束才能获取到
    //所以先填后面的内容
    //向后移动8824直接
    char *index = buffer + 32+FILTER_SIZE/8;
    //循环遍历
    while(true){
        //将键值添加到布隆过滤器
        blfter->add(cur->key);
        //写入index里
        *(uint64_t*)index = cur->key;
        index += 8;
        //写偏置(即当前的length)
        *(uint64_t*)index = vlog.vlogLength;
        index += 8;
        //写长度
        vlen_type vlen=(cur->val).length();
        if(cur->val=="~DELETED~")vlen=0;
        *(uint32_t*)index = vlen;
        index += 4;
        //写进Entry容器里
        entries.push_back(Entry(cur->key,vlen,cur->val));
        //赋值付给sstablecache
        cache->indexes.push_back(Index(cur->key,vlog.vlogLength,vlen));
        //修改vlog的长度
        //15字节的固定长度和不固定的长度value length
        vlog.vlogLength+=(15+cur->val.length());
        //判断是否还需要继续循环
        if(cur->right)cur=cur->right;
        else break;
    }
    //写完sstable还得写进vlog,以及把sstable写进文件里
    vlog.saveVlog(entries);
    //写最大值
    *(uint64_t*)(buffer + 24) = cur->key;
    (cache->header).maxKey = cur->key;
    //将布隆过滤器保存为字符串
    blfter->save2Buffer(buffer + 32);
    //将文件名存入sstablecache
    cache->path=filename;
    //写文件(不用app,因为本来就是新的)
    std::ofstream file(filename,std::ios::out|std::ios::binary);
    file.write(buffer,listSize);
    //删除额外的空间
    delete []buffer;
    file.close();
    return cache;
}
