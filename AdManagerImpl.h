#pragma once
#include "AdManager.h"
#include "Logger.h"

#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>

void gb2312ToUTF8(Message& msg);
void utf8ToGB2312(Message& msg);
Ad&  utf8ToGB2312(Ad& ad);

class CPlayer;
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
		, std::string barId			// 网吧ID
		, bool isBarServer			// 是否是网吧服务端
		, int listenPort			// 网吧服务端的监听端口
		, const std::string& logLvl);// 日志级别

	void bgnBusiness();				// 开始广告业务

	void endBusiness();				// 停业广告业务

public:
	void initResponseBuf();

	// 向广告中心请求数据
	bool requestAd(int adId);
	void requestAdList();
	void requestAdPlayPolicy();
	void downloadAds();
	void downloadAd(uint32_t id);

public:
	CPlayer *_pPlayer;
	HWND _hwnd;
	std::string _barId;
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

	bool _isEndBusiness;
	boost::asio::deadline_timer _timerPolicy;
	boost::asio::deadline_timer _timerAdList;
	boost::asio::deadline_timer _timerDownload;

public:
	boost::mutex _mutex;
	AdPlayPolicy _policy;						// 广告策略
	std::unordered_map<uint32_t, Ad> _mapAd;	// 广告信息
	std::set<uint32_t> _lockAds;
	boost::shared_ptr<std::string> _bufPolicy;	// Policy的Message(不包含msgID)的序列化值, 为了使用boost::atomic_store，而不用std::shared_ptr
	boost::shared_ptr<std::string> _bufAdList;	// AdList的Message(不包含msgID)的序列化值
	std::unordered_map<uint32_t, boost::shared_ptr<std::string>> _bufImages;	// AdFile的Message(不包含msgID)的序列化值

	src::severity_channel_logger<SeverityLevel> _logger;
};
