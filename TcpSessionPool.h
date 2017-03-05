#pragma once
#include "Logger.h"
#include "TcpSession.h"

#include <boost/thread/mutex.hpp>
#include <set>
		
class TcpSessionPool
{
public:
	/// Create instance
	static TcpSessionPool* instance();

	/// Add the specified session to the manager and start it.
	void start(SessionPtr session);

	/// Stop the specified connection.
	void stop(SessionPtr session);

	/// Stop all session.
	void stopAll();

	/// Get session pool
	std::set<SessionPtr>& getSessionPool();

private:
	TcpSessionPool();
	~TcpSessionPool();
	//TcpSessionPool(const TcpSessionPool&) = delete;	// vc2010²»Ö§³Ö
	//TcpSessionPool& operator=(const TcpSessionPool&) = delete;

	boost::mutex _mutex;
	std::set<SessionPtr> _sessionPool;
};

inline std::set<SessionPtr>& TcpSessionPool::getSessionPool()
{
	return _sessionPool;
}
