#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include "httplib.h"
#include "../searcher/searcher.h"

int main()
{
    using namespace httplib;

    //1. 创建Searcher对象
    searcher::Searcher searcher;
    bool ret = searcher.Init("../data/tmp/raw_input");
    if (!ret)
    {
        cout << "Searcher初始化失败" << endl;
        return 1;
    }

    cout << "正在启动HTTP服务......" << endl;
    char buf[200] = { '#' };
	int i = 0;
    char p[] = {'|', '/', '-', '\\'};
	for (i = 0; i < 100; i++)
	{
		buf[i] = '#';
		printf("[%-100s][%d%%][%c]\r", buf, i + 1, p[i % 4]);
		fflush(stdout);
		usleep(35000);
	}
	printf("\n");
    cout << "启动成功！" << endl;

    cout << "正在侦听: [IP: x.x.x.x] [Port: x]" << endl;

    Server server;
    server.Get("/searcher", [&searcher](const Request& req, Response& resp){
        if (!req.has_param("query"))
        {
            resp.set_content("请求参数错误", "text/plain; charset=utf-8");
            return;
        }

        string query = req.get_param_value("query");
        cout  << "收到查询词：" << query << endl;
        string results;
        searcher.Search(query, &results);
        resp.set_content(results, "application/json; charset=utf-8");

    });

    server.set_base_dir("./www");
    server.listen("172.21.0.17", 12321);
    return 0;
}
