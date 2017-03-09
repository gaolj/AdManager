#pragma once
#include "Logger.h"
#include "Message.pb.h"

#include <boost/atomic.hpp>				// boost::atomic_int
#include <boost/asio/ip/tcp.hpp>		// tcp::socket, tcp::acceptor
#include <boost/thread/future.hpp>		// boost::shared_future, boost::promise
#include <boost/thread/mutex.hpp>		// boost::mutex, boost::unique_lock
#include <boost/date_time/posix_time/posix_time_types.hpp>	// boost::mutex, boost::unique_lock
#include <list>

typedef std::function<void(Message msg)> requestHandler;

struct RequestCtx
{
	uint64_t msgID;
	std::string method;
	boost::posix_time::ptime reqTime;
	boost::promise<Message> prom;
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

	boost::future<Message> request(Message msg);	// ��Ϊ�ͻ�����������
	std::function<void(Message msg)> _requestHandler;// ����ͻ��˵�����
	void writeMsg(const Message& msg);				// ��������
	void handleNetError(const boost::system::error_code& ec);
	std::function<void()> _afterNetError;
protected:
	void keepAlive();
	void readHead();					// ����head
	void readBody(uint32_t bodyLen);	// ����body

	boost::mutex _mutex;
	boost::atomic_int _msgID;			// �������Ϣ���к�
	std::list<RequestCtx> _lstRequestCtx;// ��û���յ���Ӧ������
	std::pair<bool, boost::promise<Message>> findReqPromise(const Message& msg);

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