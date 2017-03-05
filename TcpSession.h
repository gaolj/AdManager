#pragma once
#include "Logger.h"
#include "Message.pb.h"

#include <boost/asio/ip/tcp.hpp>		// tcp::socket, tcp::acceptor
#include <boost/atomic.hpp>
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

	void start();
	void stop();

	boost::future<Message> request(Message msg);	// ��Ϊ�ͻ�����������
	void setRequestHandler(requestHandler handler);	// ��Ϊ������������
	void writeMsg(const Message& msg);				// ��������
	void handleNetError(const boost::system::error_code& ec);
	std::function<void()> _afterNetError;
protected:
	void keepAlive();
	void readHead();					// ����head
	void readBody(int bodyLen);			// ����body
	requestHandler _requestHandler;		// �������ҵ�����߼�

	boost::mutex _mutex;
	boost::atomic_int _msgID;			// �������Ϣ���к�
	std::unordered_map<uint64_t, boost::promise<Message>> _reqPromiseMap;	// ��û���յ���Ӧ������

	int _readLen;
	int _writeLen;
	char _readBuf[1024 * 1024 * 10];
	char _writeBuf[1024 * 1024 * 10];
	boost::asio::ip::tcp::socket _socket;

	std::string _peerAddr;
	logging::attribute_set::iterator _peerAddrAttr;
	src::severity_channel_logger<SeverityLevel> _logger;
};

typedef std::shared_ptr<TcpSession> SessionPtr;
