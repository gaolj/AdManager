#pragma once
#include "TcpSession.h"
#include "Logger.h"
//#include <boost/asio.hpp>

class TcpClient
{
public:
	TcpClient(boost::asio::io_service& ios);
	virtual ~TcpClient();

	void connect(const boost::asio::ip::tcp::endpoint& endpoint);
	bool waitConnected();
	SessionPtr session();
	void stop() { _session->stop(); }

private:
	boost::promise<bool> _promiseConn;
	boost::asio::ip::tcp::endpoint _endpoint;
	boost::asio::deadline_timer _reconnectTimer;	// 重连定时器
	int _reconnectInterval;							// 重连时间间隔（秒）：2，4，8，16，32，64...最多不大于10分钟

	SessionPtr _session;
	src::severity_channel_logger<SeverityLevel> _logger;
};


