// AdTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "AdManager.h"
#include <utility>

void test()
{
	try
	{
		std::string s = "abc";
		std::pair< std::string, int> a = std::make_pair(s, 7);
//		std::pair< std::string, int> b = std::make_pair< std::string, int> (s,7);
		std::pair< std::string, int> d = std::pair< std::string, int> (s, 7); 

		std::unordered_map<std::string, std::string> mymap;
		std::string aaa = "aaa";
		std::string bbb = "bbb";
//		mymap.emplace(aaa, aaa);
//		auto it1 = mymap.emplace(aaa, bbb);	// vc2010 error
		auto it2 = mymap.insert(std::make_pair(aaa, bbb));

		AdManager& adManager = AdManager::getInstance();
		adManager.setConfig("139.224.61.179", 8888, 123456, true, 18888);
		adManager.bgnBusiness();

		//adManager.downloadAd("http://139.224.61.179/%E4%B9%9D%E9%98%B4%E7%9C%9F%E7%BB%8F.wmv");
		//AdPlayPolicy policy = adManager.getAdPlayPolicy();
		//std::cout << policy.id() << policy.version() << policy.datetime() << std::endl;
		//Ad ad1 = adManager.getAd(101);
		//std::cout << ad1.id() << ad1.name() << ad1.advertiser() << std::endl;
		//Ad ad2 = adManager.getAd(101);
		//std::cout << ad1.id() << ad1.name() << ad1.advertiser() << std::endl;

		int i;
		std::cin >> i;

	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	test();
	return 0;
}

