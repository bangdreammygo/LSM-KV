#ifndef VLOG_H
#define VLOG_H
#include "dataStructure.h"
#include <cstdint>
#include <vector>
#include <random>
#include <string>
#include <iostream>
#include <list>
#include <utility>
#include "SSTable.h"
#include "skiplist.h"
#include <stdio.h>
#include "utils.h"
#include <fstream>
#include <sys/stat.h>
#include <filesystem> 
// 键值分离里存放值的地方
class vlogType
{
public:
   //记录目前的vlog的文件路径
   std::string ValuePath;
   //记录目前的vlog长度(每个entry是15byte+value长度)
   uint64_t vlogLength;
   //通过position获取value
   value_type getValue(Position pos){
      //position里有两个要素，一个是offset，一个是vlen
      std::string buffer(pos.Vlen,'\0');
      //先找到对应的文件
      std::string filename=ValuePath;
      //打开文件
      std::ifstream file(filename,std::ios::binary);
      //读取指定偏置的指定长度的值
      file.seekg(pos.Offset+15,std::ios::beg);
      file.read(&buffer[0],pos.Vlen);
      file.close();
      return buffer;
   }
   //构造函数
   vlogType(std::string path)
   {
      //将文件路径赋值
      ValuePath=path;
      //查看是否文件存在
      std::filesystem::path p(path);
      if (std::filesystem::exists(p)) {  
         //存在则需要读取大小
         auto file_size = std::filesystem::file_size(p);
         vlogLength=file_size;
      }
      else{
         vlogLength=0;
      } 
   }
   //reset，清空vlog
   void reset(){
      utils::rmfile(ValuePath);
      vlogLength=0;
   }
   //将一众value写入vlog
   void saveVlog(const std::vector<Entry> data){
      //获取到需要写入的数据
      //将文件名存到一个固定字符串里
      std::string filename=ValuePath;
      //然后就可以打开文件进行写入操作了
      std::ofstream file(filename,std::ios::app|std::ios::binary|std::ios::out);
      //然后就是无情的向该文件发起追加
      for(int i=0;i<data.size();i++){
         Entry tmp=data[i];
         //先写入magic
         file.write(reinterpret_cast<const char*>(&(tmp.Magic)),sizeof(tmp.Magic));
         //再写入校验和
         file.write(reinterpret_cast<const char*>(&(tmp.CheckSum)),sizeof(tmp.CheckSum));
         //再写入key
         file.write(reinterpret_cast<const char*>(&(tmp.Key)),sizeof(tmp.Key));
         //再写入vlen
         file.write(reinterpret_cast<const char*>(&(tmp.Vlen)),sizeof(tmp.Vlen));
         //再写入最重要的value值
         file.write(tmp.Value.c_str(),tmp.Value.length());
      }
      //结束写入
      file.close();
   }
};



#endif