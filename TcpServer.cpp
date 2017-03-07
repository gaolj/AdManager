#include "TcpServer.h"
#include "AdManager.h"
#include "TcpSession.h"
#include <boost/foreach.hpp>

using boost::asio::ip::tcp;
using boost::mutex;
using boost::unique_lock;

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

	unique_lock<mutex> lck(_mutex);
	BOOST_FOREACH(auto session, _sessionPool)					// for (auto session : _sessionPool)
		session->stopSession();
	_sessionPool.clear();
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

			startSession(session);
		}
		else
			LOG_ERROR(_logger) << "async_accept:" << ec.message();

		doAccept();
	});
}

void TcpServer::startSession(std::shared_ptr<TcpSession> session)
{
	session->_afterNetError = [this, session]()
	{
		unique_lock<mutex> lck(_mutex);
		_sessionPool.erase(session);
	};

	unique_lock<mutex> lck(_mutex);
	_sessionPool.insert(session);
	session->startSession();
}
