//
// Created by asus on 2024/4/6.
//

#ifndef SKIPLIST_MODEL_DATASTRUCTURE_H
#define SKIPLIST_MODEL_DATASTRUCTURE_H
#include <stdint.h>
#include <string>
#include <limits.h>
#include <cstdint>
#include "utils.h"
//把前置的工作都做好，把数据类型统一管理
using key_type = uint64_t;
using value_type = std::string;
using magic_type= uint8_t;
using check_type= uint16_t;
using offset_type= uint64_t;
using vlen_type= uint32_t;

//跳表的节点类
struct node
{
    //跳表的方向
    node*right, *down;   //向右向下足矣
    key_type key;
    value_type val;
    //构造函数
    node(node *right, node *down, key_type key, value_type val): right(right), down(down), key(key), val(val){}
    node(): right(nullptr), down(nullptr) {}
};


//关于sstable需要定义两种类：
//一种是常规的sstable里面存放正常的数据：header,布隆过滤器和key-offset-vlen组
//另一种是用来从内存里放进lsmkv的链表（用一个vector存放一堆的sstable链表），要记录next来找到下一个sstable
// 需要的结构：header
//header中存放时间戳，键值对数量，最大最小值
struct Header
{
   //64位的时间戳
   key_type timeStamp;
   //键值对数量
   key_type size;
   //最大最小
   key_type maxKey;
   key_type minKey;
   Header(): timeStamp(0), size(0), minKey(0), maxKey(0){}
};



//另一个需要的结构是Index结构，一个index里面存放了key指值，它的offset和value的长度
struct Index
{
    //键值
    key_type Key;
    //对应的value在vlog中的offset
    offset_type Offset;
    //对应value长度
    vlen_type Vlen;
    //无参构造函数
    Index(key_type k=0,offset_type off=0,vlen_type v=0):Key(k),Offset(off),Vlen(v){}
};
//还需要一个数据结构来封装从sstable里查找得到的值
struct Position
{
   //value在对应vlog里的偏置
   offset_type Offset;
   //数据长度
   vlen_type Vlen;
   //查找结果是否存在(默认为真)
   bool IsExist;
   //构造函数
   Position(offset_type Off=0,vlen_type V=0):Offset(Off),Vlen(V),IsExist(true){}
};




//vlog所需要的 Entry结构
struct  Entry
{
   //1byte的magic校验位
   magic_type Magic;
   //checksum
   check_type CheckSum;
   //键值
   key_type Key;
   //数据长度
   vlen_type Vlen;
   //数据值
   value_type Value;
   //记录这个entry的长度
   uint64_t Length;
   //构造函数
   Entry(key_type k=0, vlen_type vlen=0 , value_type val=0){
      Magic=0xff;
      Key=k;
      Vlen=val.length();
      if(val=="~DELETED~")Vlen=0;
      Value=val;
      //把这三个转成char的vector
      std::vector<unsigned char>buffer;
      for(int i=0;i<8;i++){
        unsigned char ch=reinterpret_cast<const unsigned char*>(&Key)[i];
        buffer.push_back(ch);
      }
      for(int i=0;i<4;i++){
        unsigned char ch=reinterpret_cast<const unsigned char*>(&Vlen)[i];
        buffer.push_back(ch);
      }
      for(int i=0;i<Value.length();i++){
        buffer.push_back(static_cast<unsigned char>(Value[i]));
      }
      CheckSum=utils::crc16(buffer);
      Length=15+Value.length();
   }
};



//在compact的时候需要的range结构：
//记录key的覆盖范围
struct Range
{
    uint64_t min, max;
    Range(const uint64_t &i, const uint64_t &a): min(i), max(a) {}
};


#endif //SKIPLIST_MODEL_DATASTRUCTURE_H
