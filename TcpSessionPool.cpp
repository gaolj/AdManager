#include "TcpSessionPool.h"
#include <boost/thread/once.hpp>
#include <boost/foreach.hpp>

using boost::once_flag;
using boost::call_once;
using boost::mutex;
using boost::unique_lock;

TcpSessionPool::TcpSessionPool()
{
}

TcpSessionPool::~TcpSessionPool()
{
}

TcpSessionPool* TcpSessionPool::instance()
{
	static once_flag instanceFlag;
	static TcpSessionPool* pInstance;

	call_once(instanceFlag, []()
	{
		static TcpSessionPool instance;
		pInstance = &instance;
	});
	return pInstance;
}

void TcpSessionPool::start(SessionPtr session)
{
	unique_lock<mutex> lck(_mutex);
	session->_afterNetError = [this, session]()
	{
		unique_lock<mutex> lck(_mutex);
		_sessionPool.erase(session);
	};
	_sessionPool.insert(session);
	session->start();
}

void TcpSessionPool::stop(SessionPtr session)
{
	unique_lock<mutex> lck(_mutex);
	_sessionPool.erase(session);
	//c->stop();
}

void TcpSessionPool::stopAll()
{
	unique_lock<mutex> lck(_mutex);
	BOOST_FOREACH(auto session, _sessionPool)					// for (auto session : _sessionPool)
		session->stop();
	_sessionPool.clear();
}

