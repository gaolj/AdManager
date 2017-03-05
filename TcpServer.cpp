#include "TcpServer.h"
#include "TcpSessionPool.h"
#include "AdManager.h"

using boost::asio::ip::tcp;

TcpServer::TcpServer(boost::asio::io_service& ios, int port):
	_ios(ios),
	_acceptor(ios, tcp::endpoint(tcp::v4(), port)),
	_logger(keywords::channel = "net")
{
}


TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
	doAccept();
}

void TcpServer::stop()
{
	_acceptor.close();
}

void TcpServer::doAccept()
{
	SessionPtr session(new TcpSession(_ios));

	_acceptor.async_accept(session->socket(),
		[this, session](boost::system::error_code ec)
	{
		BOOST_LOG_NAMED_SCOPE("handleAccept");
		FuncTracer tracer;

		if (!ec)
		{
			session->setRequestHandler(
				boost::bind(&AdManager::handleRequest, &AdManager::getInstance(), session, _1));

			TcpSessionPool::instance()->start(session);
		}
		else
			LOG_ERROR(_logger) << "async_accept:" << ec.message();

		doAccept();
	});
}
