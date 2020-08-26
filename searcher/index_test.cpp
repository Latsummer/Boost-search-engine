#include "searcher.h"

int main()
{
    searcher::Index index;
    bool ret = index.Build("../data/tmp/raw_input");
    if( !ret )
    {
        cout << "构建索引失败" << endl;
        return 1;
    }
    //构建索引成功，就调用索引相关函数（查正排，查到排）
    auto* inverted_list = index.GetInvertedList("filesystem");
    for(const auto& weight : *inverted_list)
    {
        cout << "doc_id: " << weight.doc_id
          << "weight: " << weight.weight << endl;
        auto* doc_info = index.GetDocInfo(weight.doc_id);
        cout << "tittle: " << doc_info->title << endl;
        cout << "url :" << doc_info->url << endl;
        cout << "content: " << doc_info->content << endl;
        cout << "=============================================================" << endl;
    }
}
