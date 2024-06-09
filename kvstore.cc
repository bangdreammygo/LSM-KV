#include "kvstore.h"
#include <string>


//第一个参数是文件路径，第二个参数是vlog的路径
KVStore::KVStore(const std::string &dir, const std::string &vlog) : KVStoreAPI(dir, vlog)
{
	//记录文件路径
	this->dir=dir;
	//生成一个跳表
	memtable=new skiplist();
	//生成一个vlog表
	Vlog=new vlogType(vlog);
	//初始化currenttime
	currentTime=0;
    ///////////////////////////////////////最后的任务：持久化//////////////////
    if (utils::dirExists(dir)) {
        std::vector<std::string> levelNames;
        int levelNum = utils::scanDir(dir, levelNames);
        if (levelNum > 0) {
            for(int i = 0; i < levelNum; ++i) {
                std::string levelName = "level-" + std::to_string(i);
                // check if the level directory exists
                if(std::count(levelNames.begin(), levelNames.end(), levelName) == 1) {
                    cache.push_back(std::vector<SSTableCache*>());

                    std::string levelDir = dir + "/" + levelName;
                    std::vector<std::string> tableNames;
                    int tableNum = utils::scanDir(levelDir, tableNames);

                    for(int j = 0; j < tableNum; ++j) {
                        SSTableCache* curCache = new SSTableCache(levelDir + "/" + tableNames[j]);
                        uint64_t curTime = (curCache->header).timeStamp;
                        cache[i].push_back(curCache);
                        if(curTime > currentTime)
                            currentTime = curTime;
                    }
                    // make sure the timeStamp of cache is decending
                    std::sort(cache[i].begin(), cache[i].end(), cacheTimeCompare);
                } else
                    break;
            }
        }
		else {
            utils::mkdir((dir + "/level-0").c_str());
            cache.push_back(std::vector<SSTableCache*>());
        }
    }
	//否则就直接初始化
	else {
        utils::mkdir(dir.c_str());
        utils::mkdir((dir + "/level-0").c_str());
        cache.push_back(std::vector<SSTableCache*>());
    }
	//初始化时间戳
	currentTime++;
}

KVStore::~KVStore()
{
	delete memtable;
	delete Vlog;
	for(auto it1 = cache.begin(); it1 != cache.end(); ++it1) {
        for(auto it2 = (*it1).begin(); it2 != (*it1).end(); ++it2)
            delete (*it2);
    }
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
	//插入前先检查是否内存满了
	//没存满那就继续写进去
	if(!memtable->isFull()){memtable->put(key,s);return;}
	//存满了那就先写进磁盘再继续
	//先看看零层是不是被删除了(reset掉了)
	//重建level-0
	if(!utils::dirExists(dir+"/level-0"))utils::mkdir(dir+"/level-0");
	//写入第零层
	cache[0].push_back(memtable->save2SSTable(dir+"/level-0",currentTime++,*Vlog));
	//排序
	std::sort(cache[0].begin(), cache[0].end(), cacheTimeCompare);
	//compact
	compact();
	//清空memtable
	delete memtable;
	memtable=new skiplist();
	//再插入
	memtable->put(key,s);
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key)
{
	//先找memtable
	std::string*val=memtable->get(key);
    if(val!=nullptr) {
        if(*val == "~DELETED~")return "";
        return *val;
    }
	//找不到就在sstable里找
	//循环查找
	int num=cache.size();
    for(int j=0;j<num;j++){
	  for(auto i=cache[j].begin();i!=cache[j].end();i++){
		if((*i)->isKeyNotExist(key))continue;
		//查找成功
		if((*i)->get(key).IsExist){
			//获取坐标
			Position pos=(*i)->get(key);
			if(pos.Vlen!=0)
			{value_type value=Vlog->getValue(pos);
              return value;
			}
			else return "";
		}
	  } 
	}
	return "";
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
	value_type val=get(key);
	if(val=="")return false;
	put(key, "~DELETED~");
    return true;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
	//清空跳表
	delete memtable;
	memtable=new skiplist();
    //重置vlog
	Vlog->reset();
	//清空sstable
	for(auto it1 = cache.begin(); it1 != cache.end(); it1++) {
        for(auto it2 = (*it1).begin(); it2 != (*it1).end(); it2++)
            {
				//清除缓存
				delete (*it2);
			}
    }
	////////////////////文件的删除(后续也不需要再修改)/////////////
    std::string path = dir;
    std::vector<std::string> name_dir;
    utils::scanDir(path, name_dir);
    for(auto itr1=name_dir.begin();itr1!=name_dir.end();++itr1){
        std::string sub_path = path + "/" + *itr1;
        std::vector<std::string> name_file;
        utils::scanDir(sub_path, name_file);
        for(auto itr2=name_file.begin();itr2!=name_file.end();++itr2){
            std::string file_path = sub_path + "/" + *itr2;
            utils::rmfile(file_path.c_str());
        }
        utils::rmdir(sub_path.c_str());
    }
    cache.clear();
	cache.push_back(std::vector<SSTableCache*>());

	//将时间计时器归零
	currentTime=1;

	//打印一下结束标志
	std::cout<<"I have reset your LSMKV\n";
}





// 最灵魂的操作————compact
void KVStore::compact()
{
    uint64_t levelMax = 1;
    uint32_t levelNum = cache.size();
    for(uint32_t i = 0; i < levelNum; ++i) {
        levelMax *= 2;
        //从上往下递归compact
		if(cache[i].size() > levelMax)
            compactLevel(i);
        else
            break;
    }
}
// 实际的递归函数
void KVStore::compactLevel(uint32_t level)
{
    std::vector<Range> levelRange;
    std::vector<SSTable> tables2Compact;
	//从level-0开始向下compact

	//找到sstable，并且范围确定出来
	//如果是第零层,要全部拿来compact
	if(level == 0) {
        for(auto it = cache[level].begin(); it != cache[level].end(); ++it) {
            //获取这层level的所有range
			levelRange.push_back(Range(((*it)->header).minKey, ((*it)->header).maxKey));
            //将待compact的sscache放进vector
			tables2Compact.push_back(SSTable(*it));
        }
		//将这一层给清空
        cache[level].clear();
    }
	//如果不是第零层
	else {

        uint64_t mid = 1 << level;
        auto it = cache[level].begin();
        for(uint64_t i = 0; i < mid; ++i)++it;

        uint64_t newestTime = ((*it)->header).timeStamp;
        while(it != cache[level].begin()) {
            if(((*it)->header).timeStamp > newestTime)
                break;
            --it;
        }
        if(((*it)->header).timeStamp > newestTime)
            ++it;
        while(it != cache[level].end()) {
            levelRange.push_back(Range(((*it)->header).minKey, ((*it)->header).maxKey));
            tables2Compact.push_back(SSTable(*it));
            it = cache[level].erase(it);
        }
    }
    //查看下一层是否存在，是否需要新开一层
	level++;
	if(level<cache.size()){
		//存在下一层
	    for(auto it = cache[level].begin(); it != cache[level].end();) {
            //找出有范围交集的部分
			if(haveIntersection(*it, levelRange)) {
                tables2Compact.push_back(SSTable(*it));
                it = cache[level].erase(it);
            }
			else {
                ++it;
            }
        }
	}
	//否则新开一层
	else{
        utils::mkdir((dir+ "/level-" + std::to_string(level)).c_str());
        cache.push_back(std::vector<SSTableCache*>());
	}
	//删除被compact的部分文件
    for(auto it = tables2Compact.begin(); it != tables2Compact.end(); ++it)
        utils::rmfile((*it).path.c_str());
	//按时间顺序排序
	sort(tables2Compact.begin(), tables2Compact.end(), tableTimeCompare);
	//merge这部分sstable
	SSTable::merge(tables2Compact);
	//merge结束后得到的是一个巨大的sstable，需要切割并写文件(将文件夹传过去)
	std::vector<SSTableCache*> newCaches = tables2Compact[0].save(dir + "/level-" + std::to_string(level));
    //写给下一层(注意此时的level已经被++过了)
	for(auto it = newCaches.begin(); it != newCaches.end(); ++it) {
        cache[level].push_back(*it);
    }
	//最后重新sort出结果
    std::sort(cache[level].begin(), cache[level].end(), cacheTimeCompare);
}



/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list)
{
	//scan的思路：
	//跳表底部维护一个指针即可，这个很容易实现
	//但是对于sstable该怎么处理？
	//对于每一层而言，每一层的所有cache是一个有序的序列
	//考虑：把每一层都scan一遍，得到数条scan的链表或者vector也行
	//然后再在Kvstore下，把这几条链表都维护一个指针
	//始终保持一个指针内存当前的最小的值(只存key)
	//在自顶向下移动的过程中如果有相同的值，则将那个指针后移一位，因为已经失效了
	//最后归并成一条有序链表


	//记录目前scan到了哪些key
	std::vector<key_type>allKeys;
	//从上到下开始遍历scan
	int num=cache.size();
    for(int j=0;j<num;j++){
	  for(auto i=cache[j].begin();i!=cache[j].end();i++){
		//超出范围直接离开
		if((key1>(*i)->header.maxKey)||(key2<(*i)->header.minKey))continue;
		//否则搜索进vector里
		for(auto it=(*i)->indexes.begin();it!=(*i)->indexes.end();it++){
			//如果已经超范围了就直接退出(已经比较大的key2还大了,跳表是越向后越大的，说明已经超过了，退出就好)
			key_type res=(*it).Key;
			if(key2<res)break;
			//如果处于该区间，看看是否已经添加
			if((key1<=res)&&(key2>=res)){
               //如果不存在，则添加，否则继续筛选
			   if(std::find(allKeys.begin(), allKeys.end(), res) == allKeys.end()){
				allKeys.push_back(res);
			   }
			}
		}
	  } 
	}
	//最后把memtable里也扫一扫
	node*ptr=memtable->getPtr();
	while(ptr->right!=nullptr){
		//向右扫荡
		ptr=ptr->right;
		key_type res=ptr->key;
		//如果已经超范围了就直接退出就好
		if(res>key2)break;
		//没超范围就查看是否在范围内
		if((res>=key1)&&(res<=key2)){
		if(std::find(allKeys.begin(), allKeys.end(),res ) == allKeys.end()){
				allKeys.push_back(res);
			}
		}
	}
	//此时allKeys里面已经收集到了所有的符合条件的key
	//升序排序
	std::sort(allKeys.begin(),allKeys.end());
	//插入list
	for(int i=0;i<allKeys.size();i++){
		key_type KEY=allKeys[i];
		list.push_back(std::pair<key_type,value_type>(KEY,get(KEY)));
	}
}

/**
 * This reclaims space from vLog by moving valid value and discarding invalid value.
 * chunk_size is the size in byte you should AT LEAST recycle.
 */
void KVStore::gc(uint64_t chunk_size)
{
	//chunk_size为需要扫描的大小
	//思路：先调用seek_data找到偏移量
	//然后向后逐步移动找到第一个偏移位
	//读取到0xff后说明找到了第一个
	//然后向后移动chunksize，然后一个字节一个字节的读取，直到读取到了校验位0xff，找到结束位置

	//获取到文件
	std::string filename=Vlog->ValuePath;
	//获取seek到的offset
	//但是可能会偏小，所以要向后移动到第一0xff处
	long oft=utils::seek_data_block(filename);
	std::ifstream filepos(filename,std::ios::binary);
	//移动到描述的偏移位置
	filepos.seekg(oft,std::ios::beg);
	//读取一个字节直到校验通过
	magic_type byte;
	while(filepos.read(reinterpret_cast<char*>(&byte), sizeof(byte))){
		if(byte==0xff){
			//读取到了magic校验位，退出循环
			break;
		}
		//否则偏移位会变化
		oft++;
	}
	//退出循环时已经结束了读取，oft已经到了正确的偏移位置
	filepos.close();
	std::ifstream file(filename,std::ios::binary);
	file.seekg(oft,std::ios::beg);
	//向后读取key&value来进行校验
	uint64_t total=0;
	while(total<chunk_size){
		//第一字节为magic
		//第2~3字节为checksum
		char byte;
		file.read(reinterpret_cast<char*>(&byte), sizeof(byte));
		uint16_t check;
		file.read(reinterpret_cast<char*>(&check), sizeof(check));
		//接下来8字节是key
		key_type key;
		file.read(reinterpret_cast<char*>(&key), sizeof(key));
		//再接下来读取4字节的vlen
		vlen_type vlen;
		file.read(reinterpret_cast<char*>(&vlen), sizeof(vlen));
		//特别注意是否为delete
		//最后读取vlen字节长度的value
		//是delete的情况
		if(vlen==0){
		//将长度增加:~DELETED~(9)
		total+=(15+9);
		//同时把deleted读走
		std::string valuedel(9,'\0');
		file.read(&valuedel[0], 9);
		std::string value="";
		if(get(key)==value){
		//获取到的新值等于旧值，直接插回去
		put(key,"~DELETED~");
		    }
		}
		//不是delete
		else{
			total+=(15+vlen);
			std::string value(vlen,'\0');
		    file.read(&value[0], vlen);
			//查看是否过期
		    if(get(key)==value){
			//获取到的新值等于旧值，直接插回去
			put(key,value);
		    }
		}
	}
	//读取文件结束，关闭文件
	file.close();
	//写入第零层
	cache[0].push_back(memtable->save2SSTable(dir+"/level-0",currentTime++,*Vlog));
	//排序
	std::sort(cache[0].begin(), cache[0].end(), cacheTimeCompare);
	delete memtable;
	memtable =new skiplist();
    //compaction
    compact();
	//打洞
    utils::de_alloc_file(filename,oft,total);
}