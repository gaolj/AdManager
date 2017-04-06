#include "AdManager.h"
#include "AdManagerImpl.h"
#include "TcpSession.h"
#include "TcpClient.h"
#include "TcpServer.h"
#include "Player.h"

#include <boost/atomic.hpp>				// atomic_store
#include <boost/make_shared.hpp>		// make_shared
#include <boost/locale.hpp>				// boost::locale::conv::from_utf
#include <boost/foreach.hpp>			// BOOST_FOREACH
#include <boost/thread/once.hpp>		// boost::once_flag	boost::call_once
#include <boost/algorithm/string.hpp>	// boost::to_upper_copy
#include <boost/network/protocol/http.hpp>
#include <fstream>
#include "md5.hh"

using boost::once_flag;
using boost::call_once;
using boost::mutex;
using boost::unique_lock;

#pragma warning (disable: 4003)


Ad AdManager::getAd(int adId)
{
	return _pimpl->_mapAd[adId];
}

std::shared_ptr<std::string> AdManager::getAdFile(int adId)
{
	unique_lock<mutex> lck(_pimpl->_mutex);

	std::string str = *_pimpl->_bufImages[adId];	// 如果_pimpl->_bufImages[adId]为NULL, 要测试???
	return std::make_shared<std::string>(str);
}

std::unordered_map<uint32_t, Ad> AdManager::getAdList()
{
	return _pimpl->_mapAd;
}

AdPlayPolicy AdManager::getAdPlayPolicy()
{
	return _pimpl->_policy;
}

void gb2312ToUTF8(Message& msg)
{
	msg.set_returnmsg(boost::locale::conv::to_utf<char>(msg.returnmsg(), "gb2312"));
}
void utf8ToGB2312(Message& msg)
{
	msg.set_returnmsg(boost::locale::conv::from_utf(msg.returnmsg(), "gb2312"));
}
Ad& utf8ToGB2312(Ad& ad)
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

bool AdManager::AdManagerImpl::requestAd(int adId)
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
			LOG_ERROR(_logger) << "请求广告信息失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_ERROR(_logger) << "请求广告信息, content解析失败";
		return false;
	}
}

void AdManager::AdManagerImpl::requestAdList()
{
	if (_isEndBusiness)
		return;

	Message msgReq;
	msgReq.set_method("getAdList");

	auto fut = _tcpClient->session()->request(msgReq);
	auto msgRsp = fut.get();
	utf8ToGB2312(msgRsp);

	Result ads;
	if (msgRsp.returncode() == 0 && ads.ParseFromString(msgRsp.content()) == true)
	{
		LOG_DEBUG(_logger) << "请求广告列表成功";
		Message msgAdList;
		msgAdList.set_id(0);
		msgAdList.set_returncode(0);
		msgAdList.set_method("getAdList");
		msgAdList.set_content(msgRsp.content());
		boost::atomic_store(&_bufAdList, boost::make_shared<std::string>(msgAdList.SerializeAsString()));

		for (int i = 0; i < ads.ads_size(); i++)
		{
			auto ad = *ads.mutable_ads(i);
			utf8ToGB2312(ad);
			_mapAd.insert(std::make_pair(ad.id(), ad));
		}

		if (!_isBarServer)
		{
			BOOST_FOREACH(auto& adplay, _policy.adplays())
				if (adplay.location() == 1)	// 广告位：锁屏-1
					BOOST_FOREACH(auto id, adplay.adids())
					{
						_lockAds.insert(id);
						PlayItem item;
						item.id = id;
						item.filename = _mapAd[id].filename();
						if (_pPlayer)
							_pPlayer->_playList.push_back(item);
					}
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
		_timerAdList.async_wait(boost::bind(&AdManager::AdManagerImpl::requestAdList, this));
		if (msgRsp.returncode() != 0)
			LOG_ERROR(_logger) << "请求广告列表失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_ERROR(_logger) << "请求广告列表失败, content解析失败";
	}
}


void AdManager::AdManagerImpl::requestAdPlayPolicy()
{
	if (_isEndBusiness)
		return;

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

		Message msg;
		msg.set_id(0);
		msg.set_returncode(0);
		msg.set_method("getAdPlayPolicy");
		msg.set_content(msgRsp.content());
		boost::atomic_store(&_bufPolicy, boost::make_shared<std::string>(msg.SerializeAsString()));

		requestAdList();
	}
	else
	{
		// returncode为出错或_policy解析失败，则5分钟后重新requestAdPlayPolicy
		_timerPolicy.expires_from_now(boost::posix_time::minutes(5));
		if (msgRsp.returncode() != 0)
			LOG_ERROR(_logger) << "请求广告策略失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_ERROR(_logger) << "请求广告策略失败, content解析失败";
	}

	_timerPolicy.async_wait(boost::bind(&AdManager::AdManagerImpl::requestAdPlayPolicy, this));
}
#include <sstream>      // std::stringstream
#define ERROR_METHOD_HANDLER_NOT_EXIST	(1)
#define ERROR_DATA_NOT_EXIST			(2)

void AdManager::handleRequest(std::shared_ptr<TcpSession> session, Message msg)
{
	if (msg.method() == "getAdPlayPolicy")
	{
		LOG_DEBUG(_pimpl->_logger) << "收到广告策略请求, msgID=" << msg.id();
		session->queueRspMsg(msg.id(), boost::atomic_load(&_pimpl->_bufPolicy));
	}
	else if (msg.method() == "getAdList")
	{		
		LOG_DEBUG(_pimpl->_logger) << "收到广告列表请求, msgID=" << msg.id();
		session->queueRspMsg(msg.id(), boost::atomic_load(&_pimpl->_bufAdList));
	}
	else if (msg.method() == "getAd")
	{
		int id = 0;
		memcpy(&id, msg.content().c_str(), sizeof(id));
		LOG_DEBUG(_pimpl->_logger) << "收到广告请求, msgID=" << msg.id() << ", adID=" << id;
	}
	else if (msg.method() == "getAdFile")
	{
		int id = 0;
		memcpy(&id, msg.content().c_str(), sizeof(id));
		LOG_DEBUG(_pimpl->_logger) << "收到广告文件请求, msgID=" << msg.id() << ", adID=" << id;

		boost::shared_ptr<std::string> pStr;
		unique_lock<mutex> lck(_pimpl->_mutex);
		auto it = _pimpl->_bufImages.find(id);
		if (it != _pimpl->_bufImages.end())
			pStr = it->second;
		lck.unlock();

		if (pStr)
			session->queueRspMsg(msg.id(), it->second);
		else
		{
			msg.set_returncode(ERROR_DATA_NOT_EXIST);
			msg.set_returnmsg("AdFile not exist");
			session->queueRspMsg(msg);
		}
	}
	else
	{
		msg.set_returncode(ERROR_METHOD_HANDLER_NOT_EXIST);
		msg.set_returnmsg("method handler not exist");
		session->queueRspMsg(msg);
	}
}

void AdManager::AdManagerImpl::downloadAd(uint32_t id)
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
					LOG_ERROR(_logger) << "下载广告文件(" << ad.filename() << ")失败" << response.status();
					continue;
				}

				std::string body = boost::network::http::body(response);
				MD5 context;
				context.update((unsigned char *)body.c_str(), body.length());
				context.finalize();
				std::string md5 = context.hex_digest();	// hex_digest有泄漏
				if (boost::to_upper_copy(ad.md5()) != boost::to_upper_copy(md5))
				{
					LOG_ERROR(_logger) << "下载的广告文件(" << ad.filename() << ")md5校验失败";
					continue;
				}
				else
					LOG_DEBUG(_logger) << "下载广告文件(" << ad.filename() << ")成功";

				// server端缓存广告文件回应msg
				Message msg;
				msg.set_id(0);
				msg.set_returncode(0);
				msg.set_method("getAdFile");
				msg.set_content(body);
				unique_lock<mutex> lck(_mutex);
				_bufImages.insert(std::make_pair(id, boost::make_shared<std::string>(msg.SerializeAsString())));
				lck.unlock();

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
				LOG_ERROR(_logger) << "获取广告文件失败:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
				return;
			}

			std::string body = msgRsp.content();
			MD5 context;
			context.update((unsigned char *)body.c_str(), body.length());
			context.finalize();
			std::string md5 = context.hex_digest();	// hex_digest有泄漏
			if (boost::to_upper_copy(ad.md5()) != boost::to_upper_copy(md5))
				LOG_ERROR(_logger) << "下载广告文件(" << ad.filename() << ")的md5校验失败";
			else
			{
				LOG_DEBUG(_logger) << "下载广告文件(" << ad.filename() << ")成功";
				unique_lock<mutex> lck(_mutex);
				_bufImages.insert(std::make_pair(id, boost::make_shared<std::string>(body)));
				lck.unlock();

				if (_lockAds.find(id) != _lockAds.end())
				{
					if (_pPlayer)
					{
						_pPlayer->UpdatePlayList(id, std::make_shared<std::string>(std::move(body)));
						_pPlayer->SetMediaSourceReady(true);
					}
				}
			}
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

void AdManager::AdManagerImpl::downloadAds()
{
	if (_isEndBusiness)
		return;

	// 提取该网吧所需的所有需要下载的广告ID，策略中去重
	std::set<google::protobuf::int32> idSet;
	BOOST_FOREACH(auto& adplay, _policy.adplays())	// for (auto& adplay : _policy.adplays())
		BOOST_FOREACH(auto id, adplay.adids())		// for (auto id : adplay.adids())
		if (_mapAd[id].download_size() != 0)	// 需要下载
			idSet.insert(id);

	// 删除失效的内存中的广告文件
	unique_lock<mutex> lck(_mutex);
	for (auto it = _bufImages.begin(); it != _bufImages.end();)
		if (idSet.find(it->first) != idSet.end())
			it++;
		else
			it = _bufImages.erase(it);
	lck.unlock();

	// 下载每个广告
	BOOST_FOREACH(auto id, idSet)					// for (auto id : idSet)
		if (_bufImages.find(id) == _bufImages.end())
			downloadAd(id);

	// 如果存在下载未完成的，则过5分钟后再次下载
	BOOST_FOREACH(auto id, idSet)					// for (auto id : idSet)
		if (_bufImages.find(id) == _bufImages.end())
		{
			_timerDownload.expires_from_now(boost::posix_time::minutes(5));
			_timerDownload.async_wait(boost::bind(&AdManager::AdManagerImpl::downloadAds, this));
			break;
		}
}

void AdManager::setConfig(const std::string& peerAddr, int peerPort, int barId, bool isBarServer, int listenPort, std::string logLvl)
{
	_pimpl->setConfig(peerAddr, peerPort, barId, isBarServer, listenPort, logLvl);
}

CPlayer* AdManager::setVideoWnd(HWND hwnd)
{
	_pimpl->_hwnd = hwnd;
	if (_pimpl->_pPlayer)
		_pimpl->_pPlayer->SetVideoWindow(hwnd);
	return _pimpl->_pPlayer;
}

void AdManager::closeVideoWnd()
{
	if (_pimpl->_pPlayer)
		_pimpl->_pPlayer->Shutdown();
}

void AdManager::AdManagerImpl::setConfig(const std::string& peerAddr, int peerPort, int barId, bool isBarServer, int listenPort, const std::string& logLvl)
{
	using namespace boost::asio::ip;
	_endpoint = tcp::endpoint(address::from_string(peerAddr), peerPort);
	_barId = barId;
	_isBarServer = isBarServer;
	_listenPort = listenPort;

	SeverityLevel lvl;
	if (logLvl == "trace") lvl = trace;
	else if (logLvl == "debug") lvl = debug;
	else if (logLvl == "notify") lvl = notify;
	else if (logLvl == "info") lvl = info;
	else if (logLvl == "warn") lvl = warn;
	else if (logLvl == "error") lvl = error;
	else if (logLvl == "fatal") lvl = fatal;
	else lvl = debug;

	initLogger(lvl);
	LOG_DEBUG(_logger)
		<< (isBarServer ? "服务端" : "客户端")
		<< "	广告业务配置，peer地址:" << peerAddr
		<< "	peer端口:" << peerPort
		<< "	网吧ID:" << barId
		<< "	本地端口:" << listenPort;
}

void AdManager::bgnBusiness()
{
	_pimpl->bgnBusiness();
}
void AdManager::AdManagerImpl::bgnBusiness()
{
	if (_isBarServer)
	{
		LOG_DEBUG(_logger) << "启动广告服务端";
		_tcpServer.reset(new TcpServer(_iosNet, _listenPort));
		_tcpServer->start();
	}
	else
	{
		LOG_DEBUG(_logger) << "启动广告客户端";
		HRESULT hr = CPlayer::CreateInstance(NULL, NULL, &_pPlayer);
		if (FAILED(hr))
			LOG_ERROR(_logger) << "CPlayer::CreateInstance 失败";
	}

	_iosBiz.post(
		[this]()
	{
		while (!_tcpClient->syncConnect(_endpoint))
			boost::this_thread::sleep(boost::posix_time::minutes(5));

		requestAdPlayPolicy();
	});

}

AdManager::AdManager() :
	_pimpl(new AdManagerImpl())
{
}
AdManager::AdManagerImpl::AdManagerImpl() :
	_pPlayer(NULL),
	_isEndBusiness(false),
	_timerPolicy(_iosBiz),
	_timerAdList(_iosBiz),
	_timerDownload(_iosBiz),
	_logger(keywords::channel = "ad")
{
	initResponseBuf();

	_workNet.reset(new boost::asio::io_service::work(_iosNet));
	_workBiz.reset(new boost::asio::io_service::work(_iosBiz));
	_threadNet = boost::thread([this]() {_iosNet.run(); });
	_threadBiz = boost::thread([this]() {_iosBiz.run(); });
	_tcpClient.reset(new TcpClient(_iosNet));
}

void AdManager::AdManagerImpl::initResponseBuf()
{
	Message msgPolicy;
	msgPolicy.set_id(0);
	msgPolicy.set_returncode(ERROR_DATA_NOT_EXIST);
	msgPolicy.set_method("getAdPlayPolicy");
	msgPolicy.set_returnmsg("AdPlayPolicy not exist");
	boost::atomic_store(&_bufPolicy, boost::make_shared<std::string>(msgPolicy.SerializeAsString()));

	Message msgAdList;
	msgAdList.set_id(0);
	msgAdList.set_returncode(ERROR_DATA_NOT_EXIST);
	msgAdList.set_method("getAdList");
	msgAdList.set_returnmsg("AdList not exist");
	boost::atomic_store(&_bufAdList, boost::make_shared<std::string>(msgAdList.SerializeAsString()));
}
AdManager::~AdManager()
{
}
AdManager::AdManagerImpl::~AdManagerImpl()
{
	endBusiness();
	google::protobuf::ShutdownProtobufLibrary();
}


void AdManager::endBusiness()
{
	_pimpl->endBusiness();
}
void AdManager::AdManagerImpl::endBusiness()
{
	_isEndBusiness = true;

	if (_tcpClient)
		_tcpClient->stop();
	if (_tcpServer)
		_tcpServer->stop();

	_iosNet.stop();
	_iosBiz.stop();
	_threadNet.join();
	_threadBiz.join();

	if (_pPlayer)
	{
		_pPlayer->Shutdown();
		if (0 == _pPlayer->Release())
			_pPlayer = NULL;
	}
}