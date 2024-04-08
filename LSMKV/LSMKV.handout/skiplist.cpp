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
    std::cout<<"\nwe have delete this memtable\n";
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
        if(tmp->key==key){
            cur=tmp;
            haveFound=true;
            break;
        }
        cur=cur->down;
    }
    if(haveFound) return &(cur->val);
    else return nullptr;
}
//删除操作 返回是否被删除or是否存在
bool skiplist::remove(const key_type &key) {
    bool isExist= put(key,"~DELETE~");
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
