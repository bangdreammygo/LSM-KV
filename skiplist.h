//
// Created by asus on 2024/4/6.
//

#ifndef SKIPLIST_MODEL_SKIPLIST_H
#define SKIPLIST_MODEL_SKIPLIST_H
#include "dataStructure.h"
#include <cstdint>
#include <vector>
#include <random>
#include <string>
#include <iostream>
#include <list>
#include <utility>
#include "SSTable.h"
#include <fstream>
#include "vlog.h"
class skiplist {
  private:
    //表头
    node*head;
    //跳表中插入元素的个数
    int listLength;
    //跳表转化为sstable的大小
    //初始时只有header和布隆过滤器的长度32+8192=8224
    key_type listSize;
    //用来执行scan的指针
    node*scanptr;
    //用来时刻将scan保持在最底部
    void updateScanptr();
  public:
    //构造函数
    //初始化部分数据
    skiplist(): head(new node()),scanptr(nullptr), listLength(0), listSize(8224) {}
    //析构函数
    ~skiplist();
    //返回表中的插入元素个数
    int getLength();
    //获取转化为sstable后的大小
    key_type getSize();
    // 返回表中原来是否有该key
    bool put(const key_type &key, const value_type &val);
    //获取元素
    value_type*get(const key_type &key);
    //打印一下无节点(debug)
    void readGetResult(value_type*val);
    // 返回删除情况
    bool remove(const key_type &key);
    //帮忙翻译一下remove结果(debug)
    void readRemoveresult(bool flag);
    //scan功能
    std::list<std::pair<key_type,value_type>> scan(key_type k1,key_type k2);
    //获取底部表的表头
    node* getListHead(){
      //从head开始
      node*cur=head;
      //向下移动
      while(cur->down){
        cur=cur->down;
      }
      return cur->right;
    }
    //获取最底部的指针
    node* getPtr(){
      updateScanptr();
      return scanptr;
    }


    //看看memtable是不是已经放满了
    //现在设置就放400个
    bool isFull(){return (listLength==400);}
    //将skiplist保存为sstable(cache),第一个参数是保存的目录，第二个参数是生成的时间戳
    //理清两个“写”的区别：
    //sstable里也有save，这是为了后续的compact来“写”
    //而skiplist里的写，是将memtable里的内容写进磁盘的第一层level里
    //注意这里同时也要写vlog
    //第一阶段，全部写在level-0
    //参数1：dir:写的目录
    //参数2：当前的时间戳
    //参数3：操纵vlog的类
    SSTableCache*save2SSTable(const std::string &dir, const uint64_t &currentTime , vlogType &vlog);
};


#endif //SKIPLIST_MODEL_SKIPLIST_H
