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
	
	void setConfig(					// 设置参数
		const std::string& peerAddr	// 对方地址（广告中心地址或网吧服务端地址）
		, int peerPort				// 对方端口
		, int barId = 0				// 网吧ID
		, bool isBarServer = false	// 是否是网吧服务端
		, int listenPort = 0);		// 网吧服务端的监听端口
									
	void bgnBusiness();				// 开始广告业务

	void endBusiness();				// 停业广告业务

	Ad getAd(int adId);				// 单个广告信息

	std::string getAdFile(int adId);// 单个广告文件

	AdPlayPolicy getAdPlayPolicy();	// 广告播放策略

	std::unordered_map<uint32_t, Ad> getAdList();	// 所有广告信息

	void handleRequest(std::weak_ptr<TcpSession> session, Message msg);	// 处理网吧客户端的请求

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

