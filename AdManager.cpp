#include "AdManager.h"
#include "TcpSession.h"
#include "TcpClient.h"
#include "TcpServer.h"

#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/once.hpp>
#include <boost/network/protocol/http.hpp>
#include <fstream>
#include "md5.hh"

using boost::once_flag;
using boost::call_once;

#pragma warning (disable: 4003)

void AdManager::gb2312ToUTF8(Message& msg)
{
	msg.set_returnmsg(boost::locale::conv::to_utf<char>(msg.returnmsg(), "gb2312"));
}
void AdManager::utf8ToGB2312(Message& msg)
{
	msg.set_returnmsg(boost::locale::conv::from_utf(msg.returnmsg(), "gb2312"));
}
Ad& AdManager::utf8ToGB2312(Ad& ad)
{
	ad.set_name(boost::locale::conv::from_utf(ad.name(), "gb2312"));
	ad.set_filename(boost::locale::conv::from_utf(ad.filename(), "gb2312"));
	ad.set_advertiser(boost::locale::conv::from_utf(ad.advertiser(), "gb2312"));

	::google::protobuf::RepeatedPtrField< ::std::string>* downs = ad.mutable_download();
	for (auto it = downs->begin(); it != downs->end(); ++it)
		*it = boost::locale::conv::from_utf(*it, "gb2312");

	return ad;
}

AdManager& AdManager::getInstance()
{
	static once_flag instanceFlag;
	static AdManager* pInstance;

	call_once(instanceFlag, []()
	{
		static AdManager instance;
		pInstance = &instance;
	});
	return *pInstance;
}

bool AdManager::requestAd(int adId)
{
	Message msgReq;
	msgReq.set_method("getAd");

	int id = htonl(adId);
	char bufId[4] = {};
	memcpy(bufId, &id, 4);
	msgReq.set_content(bufId, 4);

	auto fut = _tcpClient->session()->request(msgReq);
	auto msgRsp = fut.get();
	utf8ToGB2312(msgRsp);

	Ad ad;
	if (msgRsp.returncode() == 0 && ad.ParseFromString(msgRsp.content()) == true)
	{
		LOG_DEBUG(_logger) << "请求广告信息成功";
		utf8ToGB2312(ad);
		_mapAd.insert(std::make_pair(ad.id(), ad));
		return true;
	}
	else
	{
		if (msgRsp.returncode() != 0)
			LOG_DEBUG(_logger) << "请求广告信息失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_DEBUG(_logger) << "请求广告信息, content解析失败";
		return false;
	}
}

void AdManager::requestAdList()
{
	Message msgReq;
	msgReq.set_method("getAdList");

	auto fut = _tcpClient->session()->request(msgReq);
	auto msgRsp = fut.get();
	utf8ToGB2312(msgRsp);

	Result ads;
	if (msgRsp.returncode() == 0 && ads.ParseFromString(msgRsp.content()) == true)
	{
		LOG_DEBUG(_logger) << "请求广告列表成功";
		_strAdList = msgRsp.content();
		for (int i = 0; i < ads.ads_size(); i++)
		{
			auto ad = *ads.mutable_ads(i);
			utf8ToGB2312(ad);
			_mapAd.insert(std::make_pair(ad.id(), ad));
		}

		downloadAds();
	}
	else
	{
#ifdef _DEBUG
		int timeout = 3;
#else
		int timeout = 5 * 60;
#endif
		_timerAdList.expires_from_now(boost::posix_time::seconds(timeout));
		_timerAdList.async_wait(boost::bind(&AdManager::requestAdList, this));
		if (msgRsp.returncode() != 0)
			LOG_DEBUG(_logger) << "请求广告列表失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_DEBUG(_logger) << "请求广告列表失败, content解析失败";
	}
}


void AdManager::requestAdPlayPolicy()
{
	Message msgReq;
	msgReq.set_method("getAdPlayPolicy");

	int id = htonl(_barId);
	char bufId[4] = {};
	memcpy(bufId, &id, 4);
	msgReq.set_content(bufId, 4);

	auto fut = _tcpClient->session()->request(msgReq);
	auto msgRsp = fut.get();
	utf8ToGB2312(msgRsp);
	if (msgRsp.returncode() == 0 && _policy.ParseFromString(msgRsp.content()) == true)
	{
		LOG_DEBUG(_logger) << "请求广告策略成功";
		_timerPolicy.expires_from_now(boost::posix_time::hours(6));
		_strPolicy = msgRsp.content();
		requestAdList();
	}
	else
	{
		// returncode为出错或_policy解析失败，则5分钟后重新requestAdPlayPolicy
		_timerPolicy.expires_from_now(boost::posix_time::minutes(5));
		if (msgRsp.returncode() != 0)
			LOG_DEBUG(_logger) << "请求广告策略失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_DEBUG(_logger) << "请求广告策略失败, content解析失败";
	}

	_timerPolicy.async_wait(boost::bind(&AdManager::requestAdPlayPolicy, this));
}

void AdManager::handleRequest(std::weak_ptr<TcpSession> session, Message msg)
{
	auto pSession = session.lock();
	if (!pSession)
		return;

	if (msg.method() == "getAdPlayPolicy")
	{
		LOG_DEBUG(_logger) << "收到广告策略请求";
		msg.set_content(_strPolicy);
	}
	else if (msg.method() == "getAdList")
	{		
		LOG_DEBUG(_logger) << "收到广告列表请求";
		msg.set_content(_strAdList);
	}
	else if (msg.method() == "getAd")
	{
		int id = 0;
		memcpy(&id, msg.content().c_str(), sizeof(id));
		LOG_DEBUG(_logger) << "收到广告请求, id=" << id;
	}
	else if (msg.method() == "getAdFile")
	{
		int id = 0;
		memcpy(&id, msg.content().c_str(), sizeof(id));
		LOG_DEBUG(_logger) << "收到广告文件下载请求, id=" << id;

		if (_mapImage.find(id) == _mapImage.end())
		{
			msg.set_returncode(1);
			msg.set_returnmsg("没有这个广告文件");
		}
		else
			msg.set_content(_mapImage[id]);
	}

	gb2312ToUTF8(msg);
	pSession->writeMsg(msg);
}

void AdManager::downloadAd(uint32_t id)
{
	std::unordered_map<uint32_t, Ad>::iterator it = _mapAd.find(id);
	if (it == _mapAd.end())
	{
		std::cout << "not found";
		return;
	}
	
	Ad& ad =  it->second;
	if (ad.download_size() == 0)
		return;

	if (_isBarServer)
	{
#if _MSC_VER > 1600
		// 从中心下载
		BOOST_FOREACH(auto url, ad.download())	// for (auto url : ad.download())
		{
			try
			{
				boost::network::http::client httpClient;
				boost::network::http::client::request request(url);
				request << boost::network::header("Connection", "close");
				boost::network::http::client::response response = httpClient.get(request);
				if (response.status() != 200)
				{
					LOG_DEBUG(_logger) << "下载广告文件失败:" << response.status();
					continue;
				}

				std::string body = boost::network::http::body(response);
				MD5 context;
				context.update((unsigned char *)body.c_str(), body.length());
				context.finalize();
				std::string md5 = context.hex_digest();	// hex_digest有泄漏
				if (boost::to_upper_copy(ad.md5()) != boost::to_upper_copy(md5))
				{
					LOG_DEBUG(_logger) << "下载的广告文件(" << ad.filename() << ")md5校验失败";
					continue;
				}

				_mapImage.insert(std::make_pair(id, body));
				LOG_DEBUG(_logger) << "下载广告文件(" << ad.filename() << ")成功";

				std::ofstream ofs(ad.filename(), std::ofstream::binary);
				ofs << body << std::endl;
				break;
			}
			catch (const boost::exception& ex)
			{
				LOG_ERROR(_logger) << boost::diagnostic_information(ex);
			}
			catch (const std::exception& ex)
			{
				LOG_ERROR(_logger) << ex.what();
			}
		}
#endif
	}
	else
	{
		try
		{
			// 从网吧服务器下载
			Message msgReq;
			msgReq.set_method("getAdFile");
			msgReq.set_content(&id, 4);

			auto fut = _tcpClient->session()->request(msgReq);
			auto msgRsp = fut.get();
			utf8ToGB2312(msgRsp);

			if (msgRsp.returncode() != 0)
			{
				LOG_DEBUG(_logger) << "获取广告文件失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
				return;
			}

			std::string body = msgRsp.content();
			MD5 context;
			context.update((unsigned char *)body.c_str(), body.length());
			context.finalize();
			std::string md5 = context.hex_digest();	// hex_digest有泄漏
			if (boost::to_upper_copy(ad.md5()) != boost::to_upper_copy(md5))
			{
				LOG_DEBUG(_logger) << "下载的广告文件(" << ad.filename() << ")md5校验失败";
				return;
			}

			_mapImage.insert(std::make_pair(id, body));
			LOG_DEBUG(_logger) << "下载广告文件(" << ad.filename() << ")成功";
		}
		catch (const boost::exception& ex)
		{
			LOG_ERROR(_logger) << boost::diagnostic_information(ex);
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR(_logger) << ex.what();
		}
	}
}

void AdManager::downloadAds()
{
	// 提取该网吧所需的所有需要下载的广告ID，策略中去重
	std::set<google::protobuf::int32> idSet;
	BOOST_FOREACH(auto& adplay, _policy.adplays())	// for (auto& adplay : _policy.adplays())
		BOOST_FOREACH(auto id, adplay.adids())		// for (auto id : adplay.adids())
		if (_mapAd[id].download_size() != 0)	// 需要下载
			idSet.insert(id);
	//for (int i = 0; i < _policy.adplays_size(); i++)
	//{
	//	auto& adplay = _policy.adplays(i);
	//	for (int j = 0; j < adplay.adids_size(); j++)
	//	{
	//		int id = adplay.adids(j);
	//		if (_mapAd[id].download_size() != 0)	// 需要下载
	//			idSet.insert(id);
	//	}
	//}

	// 删除失效的内存中的广告文件
	for (auto it = _mapImage.begin(); it != _mapImage.end();)
		if (idSet.find(it->first) != idSet.end())
			it++;
		else
			it = _mapImage.erase(it);

	// 下载每个广告
	BOOST_FOREACH(auto id, idSet)					// for (auto id : idSet)
		if (_mapImage.find(id) == _mapImage.end())
			downloadAd(id);

	// 如果存在下载未完成的，则过5分钟后再次下载
	BOOST_FOREACH(auto id, idSet)					// for (auto id : idSet)
		if (_mapImage.find(id) == _mapImage.end())
		{
			_timerDownload.expires_from_now(boost::posix_time::minutes(5));
			_timerDownload.async_wait(boost::bind(&AdManager::downloadAds, this));
			break;
		}
}

void AdManager::setConfig(const std::string& peerAddr, int peerPort, int barId, bool isBarServer, int listenPort)
{
	using namespace boost::asio::ip;
	_endpoint = tcp::endpoint(address::from_string(peerAddr), peerPort);
	_barId = barId;
	_isBarServer = isBarServer;
	_listenPort = listenPort;

	LOG_DEBUG(_logger)
		<< (isBarServer ? "服务端" : "客户端")
		<< "	广告业务配置，peer地址:" << peerAddr
		<< "	peer端口:" << peerPort
		<< "	网吧ID:" << barId
		<< "	本地端口:" << listenPort;
}

void AdManager::bgnBusiness()
{

	if (_isBarServer)
	{
		LOG_DEBUG(_logger) << "启动广告服务端";
		_tcpServer.reset(new TcpServer(_iosNet, _listenPort));
		_tcpServer->start();
	}
	else
		LOG_DEBUG(_logger) << "启动广告客户端";

	_tcpClient.reset(new TcpClient(_iosNet));
	_tcpClient->connect(_endpoint);
	_iosBiz.post(
		[this]()
	{
		_tcpClient->waitConnected();
		requestAdPlayPolicy();
	});

}

AdManager::AdManager() :
	_timerPolicy(_iosBiz),
	_timerAdList(_iosBiz),
	_timerDownload(_iosBiz),
	_logger(keywords::channel = "ad")
{
	initLogger();
	_workNet.reset(new boost::asio::io_service::work(_iosNet));
	_workBiz.reset(new boost::asio::io_service::work(_iosBiz));
	_threadNet = boost::thread([this]() {_iosNet.run(); });
	_threadBiz = boost::thread([this]() {_iosBiz.run(); });
}

AdManager::~AdManager()
{
	endBusiness();
}

void AdManager::endBusiness()
{
	_iosNet.stop();
	_iosBiz.stop();
	_tcpClient->stop();
	_tcpServer->stop();
}