#pragma once
#include <boost/thread/mutex.hpp>		// boost::mutex, boost::unique_lock
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <set>
#include "Logger.h"

class TcpSession;
class TcpServer
{
public:
	TcpServer(boost::asio::io_service& ios, int port);
	~TcpServer();

	void start();	// �𶯼���������������
	void stop();	// ֹͣ���������ر���������

private:
	void doAccept();

	boost::asio::io_service& _ios;
	boost::asio::ip::tcp::acceptor _acceptor;

	boost::mutex _mutex;
	std::set<std::shared_ptr<TcpSession>> _sessionPool;		// TcpServer�����TCP���ӳ�
	void startSession(std::shared_ptr<TcpSession> session);

	src::severity_channel_logger<SeverityLevel> _logger;
};

