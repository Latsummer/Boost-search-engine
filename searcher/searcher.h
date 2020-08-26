#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "cppjieba/Jieba.hpp"
using namespace std;

namespace  searcher{
/////////////////////////////////////////////////////////////////
//////索引模块//////////////////////////
///////////////////////////////

  //基本索引用到的结构
  //正排索引的基础
  //给定doc_id映射到文档内容（DocInfo对象）
  struct DocInfo
  {
    int64_t doc_id;
    string title;
    string url;
    string content;
  };

  //倒排索引是给定词，映射到包含该词语的文档id列表
  struct Weight
  {
    int64_t doc_id;//该词在哪个文档出现
    int weight;//对应的权重
    string word;//什么词
  };

  //Index类用于表示整个索引结构，并提供外部调用API
  class Index
  {
    public:
      Index();

      //提供具体接口
      //1. 查正排
      const DocInfo* GetDocInfo(int64_t doc_id);

      //2. 查倒排
      const vector<Weight>* GetInvertedList(const string& key);

      //3. 构建索引
      bool Build(const string& input_path);
      void CutWord(const string& input, vector<string>* output);
    private:
      DocInfo* BuildForward(const string& line);
      void BuildInverted(const DocInfo& doc_info);

      cppjieba::Jieba jieba;
    
    private:
      //索引结构
      //正排索引，数组下标对应到doc_id
      vector<DocInfo> _forward_index;
      //倒排索引，使用一个hash表来表示映射关系
      unordered_map<string, vector<Weight> > _inverted_index;
  };

//////////////////////////////////
/////////////////以下为搜索模块
//////////////////////////////////

  class Searcher
  {
  private:
    //搜索过程中依赖索引，需要持有索引指针
    Index* index;
  public:

      Searcher() 
        :index(new Index())
        {}
      bool Init(const string& input_path);
      bool Search(const string& query, string* results);

    private:
      string GenerateDesc(const string& content, const string& word);
  };

}//namespace searcher
