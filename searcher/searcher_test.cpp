#include "searcher.h"

int main()
{
    searcher::Searcher searcher;
    bool ret = searcher.Init("../data/tmp/raw_input");
    if(!ret)
    {
        cout << "Searcher 初始化失败" << endl;
        return 1;
    }
    while(true)
    {
        cout << "searcher> " << flush;
        string query;
        cin >> query;
        if(!cin.good())
        {
            //读到EOF
            cout << "good bye" << endl;
            break;
        }
        string results;
        searcher.Search(query, &results);
        cout << results << endl;
    }
    return 0;
}