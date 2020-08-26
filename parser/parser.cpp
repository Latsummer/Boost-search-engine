//这个代码实现预处理功能
//核心功能为：读取并分析boost文档的.html文件内容
//解析出每个文档的标题，url，正文（去除html标签）
//最红把结果输出为一个行文本文件
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "../common/util.hpp"
using namespace std;

string g_input_path = "../data/input/";//表示从哪个目录中读取boost文档中的html
string g_output_path = "../data/tmp/raw_input";//表示预处理模块的输出结果

//表示文档的结构体
struct DocInfo
{
  string tittle;//标题
  string url;//url
  string content;//正文
};

//预处理核心流程
//1. 把input目录中所有的html路径枚举出来
//2. 根据枚举出的路径依次读取每个文件的内容，并进行解析
//3. 把解析结构写入的最终的输出文件中
//
bool EnumFile(const string& input_path, vector<string>* file_list)
{
  boost::filesystem::path root_path(input_path);
  if( !boost::filesystem::exists(root_path) )
  {
    cout << "目录不存在" << endl;
    return false;
  }
  
  //迭代器使用循环实现的时候可以自动完成递归
  //把迭代器的默认构造函数生成的迭代器作为一个“哨兵”
  boost::filesystem::recursive_directory_iterator end_iter;
  for(boost::filesystem::recursive_directory_iterator it(root_path);  it != end_iter; it++)
  {
    //当前路径对应的如果是目录则跳过
    if( !boost::filesystem::is_regular_file(*it) )
      continue;
    //当前路径对应的文件如果不是html文件，跳过
    if( it->path().extension() != ".html" )
      continue;

    //把得到的路径加入到vector中
    file_list->push_back(it->path().string());
  }

  return true;
}

bool ParseTitle(const string& html, string* title)
{
  size_t begin = html.find("<title>");
  if(begin == string::npos)
  {
    cout << "标题未找到" << endl;
    return false;
  }

  size_t end = html.find("</title>");
  if(end == string::npos)
  {
    cout << "标题未找到" << endl;
    return false;
  }

  begin += string("<title>").size();
  if( begin >= end )
  {
    cout << "标题不合法" << endl;
    return false;
  }
  *title = html.substr(begin, end - begin);

  return true;
}

bool ParseUrl(const string& file_path, string* url)
{
  string url_head = "https://www.boost.org/doc/libs/1_53_0/doc/";
  string url_tail = file_path.substr(g_input_path.size());

  *url = url_head + url_tail;

  return true;
}

//针对读取出的html内容进行去标签
bool ParseContent(const string& html, string* content)
{
  bool is_content = true;
  for(auto c : html)
  {
    if( is_content )//当前是正文
    {
      if( c == '<' )//遇到了标签
        is_content = false;
      else//当前是普通字符，结果尾插到content
      {
        //此处单独处理\n，预期处理结果是一个行文本文件
        //最中结果raw_input中的每一行对应到原始的html文档
        //此时就要去掉html文件中的\n
        if(c == '\n')
          c = ' ';
      content->push_back(c);
      }
    }
    else//当前是标签
    {
      if( c == '>' )//标签结束
        is_content = true;
    }
  }
  return true;
}

bool ParseFile(const string& file_path, DocInfo* doc_info)
{
  //1. 先读取文件内容
  string html;
  bool ret = common::Util::Read(file_path, &html);
  if(!ret)
  {
    cout << "解析文件失败!" << file_path << endl;
    return false;
  }

  //2. 根据文件内容解析出标题
  ret = ParseTitle(html, &doc_info->tittle);
  if( !ret )
  {
    cout << "标题解析失败" << endl;
    return false;
  }

  //3. 根据文件路径，构造出对应的在线文档
  ret = ParseUrl(file_path, &doc_info->url);
  if( !ret )
  {
    cout << "url 解析失败" << endl;
    return false;
  }

  //4. 根据文件内容，去标签，作为doc_info中的content字段内容
  ret = ParseContent(html, &doc_info->content);
  if( !ret )
  {
    cout << "正文解析失败" << endl;
    return false;
  }
  return true;

}

void WriteOutput(const DocInfo& doc_info, ofstream& ofstream)
{
  ofstream << doc_info.tittle << "\3" << doc_info.url << "\3"
    << doc_info.content << endl;
}

int main()
{
  //1. 枚举路径
  vector<string> file_list;
  bool ret = EnumFile(g_input_path, &file_list);
  if( !ret )
  {
    cout << "枚举路径失败" << endl;
    return 1;
  }

  //2. 遍历枚举出的路径，针对每一个文件，单独处理
  ofstream raw_put(g_output_path.c_str());
  if( !raw_put.is_open() )
  {
    cout << "无法打开输出文件" << endl;
    return 1;
  }

  for( const auto& file_path : file_list )
  {
    cout << file_path << endl;
    DocInfo doc_info;
    if( !ParseFile(file_path, &doc_info) )
    {
      cout << "解析文件失败" << file_path << endl;
      continue;
    }

    //3. 把解析出的结果写入到最终输出中
    WriteOutput(doc_info, raw_put);
  }

  raw_put.close();
  return 0;
}
