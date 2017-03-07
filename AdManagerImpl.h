#pragma once
#include "AdManager.h"
#include "Logger.h"

#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>

class TcpClient;
class TcpServer;
class AdManager::AdManagerImpl
{
public:
	AdManagerImpl();
	~AdManagerImpl();

	void setConfig(					// 设置参数
		const std::string& peerAddr	// 对方地址（广告中心地址或网吧服务端地址）
		, int peerPort				// 对方端口
		, int barId = 0				// 网吧ID
		, bool isBarServer = false	// 是否是网吧服务端
		, int listenPort = 0);		// 网吧服务端的监听端口

	void bgnBusiness();				// 开始广告业务

	void endBusiness();				// 停业广告业务

	void gb2312ToUTF8(Message& msg);
	void utf8ToGB2312(Message& msg);
	Ad&  utf8ToGB2312(Ad& ad);
private:

	// 向广告中心请求数据
	bool requestAd(int adId);
	void requestAdList();
	void requestAdPlayPolicy();
	void downloadAds();
	void downloadAd(uint32_t id);

	int _barId;
	int _listenPort;
	bool _isBarServer;
	boost::asio::ip::tcp::endpoint _endpoint;

	boost::thread _threadNet;
	boost::thread _threadBiz;
	boost::asio::io_service _iosNet;
	boost::asio::io_service _iosBiz;
	std::shared_ptr<boost::asio::io_service::work> _workNet;
	std::shared_ptr<boost::asio::io_service::work> _workBiz;

	std::shared_ptr<TcpClient> _tcpClient;
	std::shared_ptr<TcpServer> _tcpServer;

	boost::asio::deadline_timer _timerPolicy;
	boost::asio::deadline_timer _timerAdList;
	boost::asio::deadline_timer _timerDownload;

public:
	// 以下成员，都在唯一的广告业务线程中访问，所以不需要同步
	AdPlayPolicy _policy;
	std::string _strPolicy;
	std::string _strAdList;
	std::unordered_map<uint32_t, Ad> _mapAd;
	std::unordered_map<uint32_t, std::string> _mapImage;

	src::severity_channel_logger<SeverityLevel> _logger;
};
