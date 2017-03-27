#pragma once
#include "TcpSession.h"
#include "Logger.h"

class TcpClient
{
public:
	TcpClient(boost::asio::io_service& ios);
	virtual ~TcpClient();

	bool syncConnect(const boost::asio::ip::tcp::endpoint& endpoint);
	void asyncConnect(const boost::asio::ip::tcp::endpoint& endpoint);

	SessionPtr session();
	void setAutoReconnect(bool autoReconnect);
	bool isConnected();
	void stop();

private:
	boost::asio::ip::tcp::endpoint _endpoint;
	boost::asio::deadline_timer _reconnectTimer;	// 重连定时器
	int _reconnectInterval;							// 重连时间间隔（秒）：2，4，8，16，32，64...最多不大于10分钟

	SessionPtr _session;
	src::severity_channel_logger<SeverityLevel> _logger;
};

inline void TcpClient::stop()
{
	setAutoReconnect(false);
	_session->stopSession();
}

inline SessionPtr TcpClient::session()
{
	return _session;
}

inline bool TcpClient::isConnected()
{
	return _session->isConnected();
}

inline void TcpClient::setAutoReconnect(bool autoReconnect)
{
	if (!autoReconnect)
		_session->_afterNetError = NULL;
	else
		_session->_afterNetError = [this]() {asyncConnect(_endpoint);};
}

