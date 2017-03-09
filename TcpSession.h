#pragma once
#include "Logger.h"
#include "Message.pb.h"

#include <boost/atomic.hpp>				// boost::atomic_int
#include <boost/asio/ip/tcp.hpp>		// tcp::socket, tcp::acceptor
#include <boost/thread/future.hpp>		// boost::shared_future, boost::promise
#include <boost/thread/mutex.hpp>		// boost::mutex, boost::unique_lock
#include <unordered_map>

typedef std::function<void(Message msg)> requestHandler;

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
	void writeMsg(const Message& msg);				// 发送数据
	void handleNetError(const boost::system::error_code& ec);
	std::function<void()> _afterNetError;
protected:
	void keepAlive();
	void readHead();					// 接收head
	void readBody(uint32_t bodyLen);	// 接收body

	boost::mutex _mutex;
	boost::atomic_int _msgID;			// 请求的消息序列号
	std::unordered_map<uint64_t, boost::promise<Message>> _reqPromiseMap;	// 还没有收到回应的请求

	uint32_t _readLen;
	uint32_t _writeLen;
	std::vector<char> _readBuf;
	std::vector<char> _writeBuf;
	boost::asio::ip::tcp::socket _socket;

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