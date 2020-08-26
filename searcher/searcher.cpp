#include "searcher.h"
#include "../common/util.hpp"
#include <fstream>
#include <boost/algorithm/string/case_conv.hpp>
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>

namespace searcher{

  const char *const DICT_PATH = "../jieba_dict/jieba.dict.utf8";
  const char *const HMM_PATH = "../jieba_dict/hmm_model.utf8";
  const char *const USER_DICT_PATH = "../jieba_dict/user.dict.utf8";
  const char *const IDF_PATH = "../jieba_dict/idf.utf8";
  const char *const STOP_WORD_PATH = "../jieba_dict/stop_words.utf8";

  Index::Index()
    : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH)
  {}

  const DocInfo* Index::GetDocInfo(int64_t doc_id)
  {
    if( doc_id < 0 || doc_id >= _forward_index.size() )
      return nullptr;
    return &_forward_index[doc_id];
  }

  const vector<Weight>* Index::GetInvertedList(const string& key)
  {
    auto it = _inverted_index.find(key);
    if( it == _inverted_index.end() )
      return nullptr;

    return &it->second;
  }


  bool Index::Build(const string& input_path)
  {
    //1. 按行读取输入文件内容（预处理模块生成的raw_input）
    //raw_input结构：行文本文件，每一行对应一个文档
    //每一行有三个部分，使用\3来切分，分别为标题，url，正文
    cerr << "开始构造索引" << endl;
    ifstream file(input_path.c_str());
    if( !file.is_open() )
    {
      cout << "rwa_input opend faiult" << endl;
      return false;
    }

    char buff[200] = {'#'};
    char p[] = {'|', '/', '-', '\\'};
    int jin = 0;
    printf("[%-99s][%d%%][%c]\r", buff, jin + 1, p[jin % 4]);

    string line;
    while(getline(file, line))
    {
    //2. 针对当前行，解析成DocInfo对象，并构造为正排索引
      DocInfo* doc_info = BuildForward(line);
      if(doc_info == nullptr)
      {
        cout << "构建正排失败" << endl;
        continue;
      }
    //3. 针对当前的DocInfo对象，进行解析，构造为倒排索引
      BuildInverted(*doc_info);

      //次数不适合直接加循环
      //需要既可以看到进度，又要尽量小的影响程序执行
      //if(doc_info->doc_id % 100 == 0)
        //cerr << doc_info->doc_id << endl;
      if(doc_info->doc_id % 59 == 0)
      {
        buff[jin] = '#';
        jin++;
        printf("[%-99s][%d%%][%c]\r", buff, jin + 1, p[jin % 4]);
        fflush(stdout);
      }
    }
    cerr << endl;
    cerr << "索引构建完毕" << endl;
    file.close();
    return true;
  }

  DocInfo* Index::BuildForward(const string& line)
  {
    vector<string> tokens;
    common::Util::Split(line, "\3", &tokens);
    if( tokens.size() != 3 )//如果没有被切分为3份，说明节分结果有问题
      return nullptr;
    //2. 把切分结果填充到DocInfo对象中
    DocInfo doc_info;
    doc_info.doc_id = _forward_index.size();
    doc_info.title = tokens[0];
    doc_info.url = tokens[1];
    doc_info.content = tokens[2];
    _forward_index.push_back(move(doc_info));//转化为右值引用，移动语义复制赋值

    return &_forward_index.back();//防止野指针问题
  }
  
  //倒排是一个hash表
  //key是词（针对文档分词结果）
  //value是倒排拉链（包含若干个Weight对象）
  //每次遍历到一个文档，分析之后把县官信息更新到倒排结构中
  //权重 = 标题出现次数 * 10 + 正文出现次数（没有什么依据，只是觉得这样还蛮不错）
  void Index::BuildInverted(const DocInfo& doc_info)
  {
    //0. 创建专门统计词频的结构
    struct WordCnt
    {
      int _title_cnt;
      int _content_cnt;

      WordCnt() : _title_cnt(0), _content_cnt(0) {}
    };
    unordered_map<string, WordCnt> word_cnt_map;

    //1. 对标题进行分词
    vector<string> title_token;
    CutWord(doc_info.title, &title_token);

    //2. 遍历分词结果，统计每个单词出现次数
    //次数要考虑大小写问题，大小写应该都算成小写
    for(string& word : title_token)
    {
      boost::to_lower(word);
      ++word_cnt_map[word]._title_cnt;
    }

    //3. 对正文分词
    vector<string> content_token;
    CutWord(doc_info.content, &content_token);

    //4. 遍历分词结果，统计每个单词出现次数
    for (string word : content_token) 
    {
       boost::to_lower(word);
       ++word_cnt_map[word]._content_cnt;                      
    }

    //5. 根据统计结果，整个出Weight对象，把结果更新到倒排索引
    for(const auto& word_pair : word_cnt_map)
    {
      Weight weight;
      weight.doc_id = doc_info.doc_id;
      weight.weight = 10 * word_pair.second._title_cnt + word_pair.second._content_cnt;
      weight.word = word_pair.first;

      vector<Weight>& inverted_list = _inverted_index[word_pair.first];
      inverted_list.push_back(move(weight));
    }
  }

  void Index::CutWord(const string& input, vector<string>* output)
  {
    jieba.CutForSearch(input, *output);
  }

  /////////////////////////////////////////////////////////////
  ///以下代码为 Searcher模块/////////////////////
  /////////////////////////////////////////////////////////////

  bool Searcher::Init(const string& input_path)
  {
    return index->Build(input_path);
  }

  //把查询词进行搜索，得到搜索结果
  bool Searcher::Search(const string& query, string* output)
  {
    //1. [分词] 针对查询结果进行分词
    vector<string> tokens;
    index->CutWord(query, &tokens);

    //2. [触发] 根据分词结果，查询倒排，把相关文档都获取到
    vector<Weight> all_token_result;
    for(string word : tokens)
    {
      //做索引的时候，已经把其中的词统一转成小写了
      //查询到排的时候，也需要把查询词统一转成小写
      boost::to_lower(word);

      auto* inverted_list = index->GetInvertedList(word);
      if( inverted_list == nullptr )
      {
        //说明该词在倒排索引中不存在，如果这个词比较生僻，
        //在所有文档中都没有出现过。此时得到的倒排拉链就是nullptr
        continue;
      }
      //tokens 包含多个结果，需要把多个结果合并到一起，才能进行统一排序
      all_token_result.insert(all_token_result.end(), 
                              inverted_list->begin(), inverted_list->end());
    }

    //3. [排序] 把刚才查到的文档的倒排拉链合并到一起并按照权重进行降序排序
    sort(all_token_result.begin(), all_token_result.end(),
    [](const Weight& w1, const Weight& w2){
      //如果要实现升序排序，w1 < w2
      //实现降序排序 w1 > w2
      return w1.weight > w2.weight;
    });

    //4. [包装结果] 把得到的这些倒排拉链中的文档id获取到，然后去查正排，
    //             再把doc_info中的内容构造成最终的预期格式(JSON)
    //使用JSONCPP库来实现
    Json::Value results;//包含若干个结果，每个结果就是一个JSON对象对象
    for(const auto& weight : all_token_result)
    {
      //根据weight中的结果查询正排序
      const DocInfo* doc_info = index->GetDocInfo(weight.doc_id);
      //把doc_info对象进一步包装成一个JSON对象
      Json::Value result;
      result["title"] = doc_info->title;
      result["url"] = doc_info->url;
      result["desc"] = GenerateDesc(doc_info->content, weight.word);
      results.append(result);
    }
    //最后一步，把得到的results这个JSON对象序列化为字符串，写入output中
    Json::FastWriter writer;
    *output = writer.write(results);

    return true;
  }
  string Searcher::GenerateDesc(const string& content, const string& word)
  {
    //例如：
      //根据正文，找到word出现的位置
      //以该位置为中心，往前找60个字节，作为描述起始位置
      //以该位置为中心，往后找160字节，作为描述结束的位置
      //需要注意边界条件：
      //1. 前面不够60字节就可以从0开始
      //2. 后面内容不够就到末尾结束
      //3. 内容不够显示就可以使用...
    
    //1. 先找到word在正文出现的位置
    size_t first_pos = content.find(word);
    size_t desc_beg = 0;
    if(first_pos == string::npos)
    {
      //说明该词在正文中不存在（例如该词只在标题中出现了）
      //如果找不到，就直接从头开始作为起始位置
      if(content.size() < 160)
        return content;
      string desc = content.substr(0, 160);
      desc[desc.size() - 1] = '.';
      desc[desc.size() - 2] = '.';
      desc[desc.size() - 3] = '.';
      return desc;
    }
    
    //2. 找到了first_pos位置，以其为基准，往前找
    desc_beg = first_pos < 60 ? 0 : first_pos - 60;
    if(desc_beg + 160 >= content.size())
    {
      //desc_beg后面的内容不够160，直接到末尾即可
      return content.substr(desc_beg);
    }
    else
    {
      string desc = content.substr(desc_beg, 160);
      desc[desc.size() - 1] = '.';
      desc[desc.size() - 2] = '.';
      desc[desc.size() - 3] = '.';
      return desc;
    }
  }

}//namespace searcher
