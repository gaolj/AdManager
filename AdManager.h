#pragma once
#include <unordered_map>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "Message.pb.h"
#include "AdPlayPolicy.pb.h"
#include "Logger.h"

class TcpClient;
class TcpServer;
class TcpSession;

class AdManager
{
public:
	static AdManager& getInstance();

	void setConfig(const std::string& peerAddr, int peerPort, int barId = 0, bool isBarServer = false, int listenPort = 0);
	void bgnBusiness();		// 开始广告业务
	void endBusiness();		// 停业广告业务

	Ad getAd(int adId);
	std::unordered_map<uint32_t, Ad> getAdList();
	AdPlayPolicy getAdPlayPolicy();

	// 处理客户机的请求
	void handleRequest(std::weak_ptr<TcpSession> session, Message msg);

private:
	AdManager();
	~AdManager();

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

	AdPlayPolicy _policy;
	std::string _strPolicy;
	std::string _strAdList;
	std::unordered_map<uint32_t, Ad> _mapAd;
	std::unordered_map<uint32_t, std::string> _mapImage;

	src::severity_channel_logger<SeverityLevel> _logger;
};

