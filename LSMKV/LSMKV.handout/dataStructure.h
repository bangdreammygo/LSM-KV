//
// Created by asus on 2024/4/6.
//

#ifndef SKIPLIST_MODEL_DATASTRUCTURE_H
#define SKIPLIST_MODEL_DATASTRUCTURE_H
#include <stdint.h>
#include <string>
#include <limits.h>
#include <cstdint>
//把前置的工作都做好，把数据类型统一管理
using key_type = uint64_t;
using value_type = std::string;



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


#endif //SKIPLIST_MODEL_DATASTRUCTURE_H
