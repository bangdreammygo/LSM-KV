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
    std::cout<<"we have delete this memtable\n";
}
//获取表长
int skiplist::getLength() {
    return listLength;
}
//获取sstable的大小
key_type skiplist::getSize() {
    return listSize;
}

//插入操作
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
        }
    }
}


//查找操作
value_type skiplist::get(const key_type &key) {
    return value_type();
}

value_type skiplist::remove(const key_type &key) {
    return value_type();
}
