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
class skiplist {
  private:
    //表头
    node*head;
    //跳表中插入元素的个数
    int listLength;
    //跳表转化为sstable的大小
    //初始时只有header和布隆过滤器的长度32+8192=8224
    key_type listSize;

  public:
    //构造函数
    //初始化部分数据
    skiplist(): head(new node()), listLength(0), listSize(8224) {}
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
};


#endif //SKIPLIST_MODEL_SKIPLIST_H
