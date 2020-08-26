#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
using namespace std;

namespace common
{
class Util
{
  public: 
    //负责从指定的路径中读取文件的整体内容，读取到output中
    static bool Read(const string& input_path, string* output)
    {
      ifstream file(input_path.c_str());
      if( !file.is_open() )
        return false;

      //按行读取，把读到的每行结果追加到output
      string line;
      while( getline(file, line) )
        *output += (line + "\n");
      file.close();

      return true;
    }

    //cut为分隔符号
    //token_compress_off表示遇到分隔符相邻时不会压缩切分结果
    static void Split(const string& input, const string& cut, vector<string>* output)
    {
      boost::split(*output, input, boost::is_any_of(cut), boost::token_compress_off);
    }
};

}//namecpace common
