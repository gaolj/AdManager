#pragma once
#include "Logger.h"
#include "Message.pb.h"

#include <boost/atomic.hpp>				// boost::atomic_bool
#include <boost/asio/ip/tcp.hpp>		// tcp::socket, tcp::acceptor
#include <boost/asio/streambuf.hpp>		// boost::asio::streambuf
#include <boost/thread/future.hpp>		// boost::shared_future, boost::promise
#include <boost/thread/mutex.hpp>		// boost::mutex, boost::unique_lock
#include <boost/date_time/posix_time/posix_time_types.hpp>	// boost::mutex, boost::unique_lock
#include <list>
#include <deque>

typedef std::function<void(Message msg)> requestHandler;

struct RequestCtx
{
	uint64_t msgID;
	std::string method;
	std::shared_ptr<std::string> wireData;
	boost::posix_time::ptime reqTime;
	std::shared_ptr<boost::promise<Message>> prom;
};

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:
	TcpSession(boost::asio::io_service& ios);
	~TcpSession();
	boost::asio::ip::tcp::socket& socket();

	void startSession();
	void stopSession();
	bool isConnected();

	boost::future<Message> request(Message msg);	// 作为客户机发出请求
	std::function<void(Message msg)> _requestHandler;// 处理客户端的请求
	void queueRspMsg(const Message& msg);			// Response msg放入队列
	void queueRspMsg(uint64_t msgID, boost::shared_ptr<std::string> data);// Response msg放入队列
	void handleNetError(const boost::system::error_code& ec);
	std::function<void()> _afterNetError;
protected:
	void keepAlive();
	void readHead();					// 接收head
	void readBody(uint32_t bodyLen);	// 接收body
	void writeData();					// 发送数据

	boost::mutex _mutex;
	uint32_t _msgID;					// 请求的消息序列号
	std::list<RequestCtx> _lstRequestCtx;// 还没有收到回应的请求
	std::pair<bool, boost::promise<Message>> findReqPromise(const Message& msg);

	boost::asio::streambuf _recvBuf;
	boost::asio::streambuf _sendBuf;
	std::deque<std::pair<std::string, boost::shared_ptr<std::string>>> _msgQueue;
	boost::asio::ip::tcp::socket _socket;
	boost::asio::io_service& _ios;

	std::string _peerAddr;
	boost::atomic_bool _isConnected;
	logging::attribute_set::iterator _peerAddrAttr;
	src::severity_channel_logger<SeverityLevel> _logger;
};
typedef std::shared_ptr<TcpSession> SessionPtr;

inline bool TcpSession::isConnected()
{
	return _isConnected;
}