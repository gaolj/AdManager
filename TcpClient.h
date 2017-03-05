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
	boost::asio::deadline_timer _reconnectTimer;	// ������ʱ��
	int _reconnectInterval;							// ����ʱ�������룩��2��4��8��16��32��64...��಻����10����

	SessionPtr _session;
	src::severity_channel_logger<SeverityLevel> _logger;
};


