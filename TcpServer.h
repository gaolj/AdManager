#pragma once
#include <boost/asio.hpp>

#include "Logger.h"

class TcpServer
{
public:
	TcpServer(boost::asio::io_service& ios, int port);
	~TcpServer();

	void start();
	void stop();

private:
	void doAccept();

	boost::asio::io_service& _ios;
	boost::asio::ip::tcp::acceptor _acceptor;

	src::severity_channel_logger<SeverityLevel> _logger;
};

