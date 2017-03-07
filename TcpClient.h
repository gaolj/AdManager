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
	void stop();

private:
	boost::asio::ip::tcp::endpoint _endpoint;
	boost::asio::deadline_timer _reconnectTimer;	// ������ʱ��
	int _reconnectInterval;							// ����ʱ�������룩��2��4��8��16��32��64...��಻����10����

	SessionPtr _session;
	src::severity_channel_logger<SeverityLevel> _logger;
};

inline void TcpClient::stop()
{
	_session->stop();
}

inline SessionPtr TcpClient::session()
{
	return _session;
}

