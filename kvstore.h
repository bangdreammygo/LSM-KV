#pragma once

#include "kvstore_api.h"
#include "skiplist.h"
#include "SSTable.h"
#include "vlog.h"
#include <algorithm>
class KVStore : public KVStoreAPI
{
	// You can add your implementation here
private:
    //首先它需要一个memtable
	skiplist*memtable;
    //然后有一个vlog?
	vlogType*Vlog;
	//有一个时间记录
	uint64_t currentTime;
	//记录文件路径path
	std::string dir;
	//有一个记录了所有层的sstable的内存空间
	std::vector<std::vector<SSTableCache*>>cache;
	//compact函数
	void compact();
    void compactLevel(uint32_t level);

public:
	KVStore(const std::string &dir, const std::string &vlog);

	~KVStore();

	void put(uint64_t key, const std::string &s) override;

	std::string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;

	void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list) override;

	void gc(uint64_t chunk_size) override;
};
