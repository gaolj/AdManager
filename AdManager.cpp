#include "AdManager.h"
#include "TcpSession.h"
#include "TcpClient.h"
#include "TcpServer.h"

#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include "md5.hh"

#pragma warning (disable: 4003)

namespace
{
	std::string get_filename(const boost::network::uri::uri &url) {
		std::string path = boost::network::uri::path(url);
		std::size_t index = path.find_last_of('/');
		std::string filename = path.substr(index + 1);
		return filename.empty() ? "index.html" : filename;
	}

	void GB2312ToUTF8(Message& msg)
	{
		msg.set_returnmsg(boost::locale::conv::to_utf<char>(msg.returnmsg(), "gb2312"));
	}
	void UTF8ToGB2312(Message& msg)
	{
		msg.set_returnmsg(boost::locale::conv::from_utf(msg.returnmsg(), "gb2312"));
	}
	Ad& UTF8ToGB2312(Ad& ad)
	{
		ad.set_name(boost::locale::conv::from_utf(ad.name(), "gb2312"));
//		ad.set_filename(boost::locale::conv::from_utf(ad.filename(), "gb2312"));
		ad.set_advertiser(boost::locale::conv::from_utf(ad.advertiser(), "gb2312"));

		::google::protobuf::RepeatedPtrField< ::std::string>* downs = ad.mutable_download();
		for (auto it = downs->begin(); it != downs->end(); ++it)
			*it = boost::locale::conv::from_utf(*it, "gb2312");

		return ad;
	}

}

AdManager& AdManager::getInstance()
{
	static AdManager instance;
		return instance;
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
	UTF8ToGB2312(msgRsp);

	if (msgRsp.returncode() != 0)
	{
		LOG_DEBUG(_logger) << "��������Ϣʧ��";
		return false;
	}

	Ad ad;
	if (ad.ParseFromString(msgRsp.content()) == true)
	{
		LOG_DEBUG(_logger) << "��������Ϣ�ɹ�";
		UTF8ToGB2312(ad);
		_mapAd.insert(std::make_pair(ad.id(), ad));
		return true;
	}
	else
	{
		LOG_DEBUG(_logger) << "��������Ϣ, content����ʧ��";
		return false;
	}
}

void AdManager::requestAdList()
{
	Message msgReq;
	msgReq.set_method("getAdList");

	auto fut = _tcpClient->session()->request(msgReq);
	auto msgRsp = fut.get();
	UTF8ToGB2312(msgRsp);

	Result ads;
	if (msgRsp.returncode() == 0 && ads.ParseFromString(msgRsp.content()) == true)
	{
		LOG_DEBUG(_logger) << "�������б�ɹ�";
		_strAdList = msgRsp.content();
		for (int i = 0; i < ads.ads_size(); i++)
		{
			auto ad = *ads.mutable_ads(i);
			UTF8ToGB2312(ad);
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
			LOG_DEBUG(_logger) << "�������б�ʧ��:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_DEBUG(_logger) << "�������б�ʧ��, content����ʧ��";
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
	UTF8ToGB2312(msgRsp);
	if (msgRsp.returncode() == 0 && _policy.ParseFromString(msgRsp.content()) == true)
	{
		LOG_DEBUG(_logger) << "��������Գɹ�";
		_timerPolicy.expires_from_now(boost::posix_time::hours(6));
		_strPolicy = msgRsp.content();
		requestAdList();
	}
	else
	{
		// returncodeΪ�����_policy����ʧ�ܣ���5���Ӻ�����requestAdPlayPolicy
		_timerPolicy.expires_from_now(boost::posix_time::minutes(5));
		if (msgRsp.returncode() != 0)
			LOG_DEBUG(_logger) << "���������ʧ��:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
		else
			LOG_DEBUG(_logger) << "���������ʧ��, content����ʧ��";
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
		LOG_DEBUG(_logger) << "�յ�����������";
		msg.set_content(_strPolicy);
	}
	else if (msg.method() == "getAdList")
	{		
		LOG_DEBUG(_logger) << "�յ�����б�����";
		msg.set_content(_strAdList);
	}
	else if (msg.method() == "getAd")
	{
		int id = 0;
		memcpy(&id, msg.content().c_str(), sizeof(id));
		LOG_DEBUG(_logger) << "�յ��������, id=" << id;
	}
	else if (msg.method() == "getAdFile")
	{
		int id = 0;
		memcpy(&id, msg.content().c_str(), sizeof(id));
		LOG_DEBUG(_logger) << "�յ�����ļ���������, id=" << id;

		if (_mapImage.find(id) == _mapImage.end())
		{
			msg.set_returncode(1);
			msg.set_returnmsg("û���������ļ�");
		}
		else
			msg.set_content(_mapImage[id]);
	}

	GB2312ToUTF8(msg);
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
		// ����������
		BOOST_FOREACH(auto url, ad.download())	// for (auto url : ad.download())
		{
			boost::network::http::client::request request(url);
			request << boost::network::header("Connection", "close");
			boost::network::http::client::response response = _httpClient.get(request);
			if (response.status() != 200)
			{
				LOG_DEBUG(_logger) << "���ع���ļ�ʧ��:" << response.status();
				continue;
			}

			std::string body = boost::network::http::body(response);
			MD5 context;
			context.update((unsigned char *)body.c_str(), body.length());
			context.finalize();
			std::string md5 = context.hex_digest();	// hex_digest��й©
			if (boost::to_upper_copy(ad.md5()) != boost::to_upper_copy(md5))
			{
				LOG_DEBUG(_logger) << "���صĹ���ļ�(" << ad.filename() << ")md5У��ʧ��";
				continue;
			}

			_mapImage.insert(std::make_pair(id, body));
			LOG_DEBUG(_logger) << "���ع���ļ�(" << ad.filename() << ")�ɹ�";

			//ad.set_filename(boost::locale::conv::from_utf(uri::decoded(get_filename(request.uri())), "gb2312"));
			//std::ofstream ofs(ad.filename(), std::ofstream::binary);
			//ofs << body << std::endl;
			break;
		}
#endif
	}
	else
	{
		// �����ɷ���������
		Message msgReq;
		msgReq.set_method("getAdFile");
		msgReq.set_content(&id, 4);

		auto fut = _tcpClient->session()->request(msgReq);
		auto msgRsp = fut.get();
		UTF8ToGB2312(msgRsp);

		if (msgRsp.returncode() != 0)
		{
			LOG_DEBUG(_logger) << "��ȡ����ļ�ʧ��:" << msgRsp.returncode() << ", " << msgRsp.returnmsg();
			return;
		}

		std::string body = msgRsp.content();
		MD5 context;
		context.update((unsigned char *)body.c_str(), body.length());
		context.finalize();
		std::string md5 = context.hex_digest();	// hex_digest��й©
		if (boost::to_upper_copy(ad.md5()) != boost::to_upper_copy(md5))
		{
			LOG_DEBUG(_logger) << "���صĹ���ļ�(" << ad.filename() << ")md5У��ʧ��";
			return;
		}

		_mapImage.insert(std::make_pair(id, body));
		LOG_DEBUG(_logger) << "���ع���ļ�(" << ad.filename() << ")�ɹ�";
	}
}

void AdManager::downloadAds()
{
	// ��ȡ�����������������Ҫ���صĹ��ID��������ȥ��
	std::set<google::protobuf::int32> idSet;
	BOOST_FOREACH(auto& adplay, _policy.adplays())	// for (auto& adplay : _policy.adplays())
		BOOST_FOREACH(auto id, adplay.adids())		// for (auto id : adplay.adids())
			if (_mapAd[id].download_size() != 0)	// ��Ҫ����
				idSet.insert(id);

	// ɾ��ʧЧ���ڴ��еĹ���ļ�
	for (auto it = _mapImage.begin(); it != _mapImage.end();)
		if (idSet.find(it->first) != idSet.end())
			it++;
		else
			it = _mapImage.erase(it);

	// ����ÿ�����
	BOOST_FOREACH(auto id, idSet)					// for (auto id : idSet)
		if (_mapImage.find(id) == _mapImage.end())
			downloadAd(id);

	// �����������δ��ɵģ����5���Ӻ��ٴ�����
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
		<< (isBarServer ? "�����" : "�ͻ���")
		<< "	���ҵ�����ã�peer��ַ:" << peerAddr
		<< "	peer�˿�:" << peerPort
		<< "	����ID:" << barId
		<< "	���ض˿�:" << listenPort;
}

void AdManager::bgnBusiness()
{

	if (_isBarServer)
	{
		LOG_DEBUG(_logger) << "�����������";
		_tcpServer.reset(new TcpServer(_iosNet, _listenPort));
		_tcpServer->start();
	}
	else
		LOG_DEBUG(_logger) << "�������ͻ���";

	_tcpClient.reset(new TcpClient(_iosNet));
	_tcpClient->connect(_endpoint);
	_iosBiz.post(
		[this]()
	{
		_tcpClient->waitConnected();
		requestAdPlayPolicy();
	});

}

Ad AdManager::getAd(int adId)
{
	return _mapAd[adId];
}

std::unordered_map<uint32_t, Ad> AdManager::getAdList()
{
	return _mapAd;
}

AdPlayPolicy AdManager::getAdPlayPolicy()
{
	return _policy;
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