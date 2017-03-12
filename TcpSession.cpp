#include "TcpSession.h"
#include "mstcpip.h"			// struct tcp_keepalive
#include <boost/asio.hpp>

using std::string;
using boost::asio::ip::tcp;
using boost::mutex;
using boost::unique_lock;

TcpSession::TcpSession(boost::asio::io_service& ios):
	_socket(ios),
	_msgID(1),
	_isConnected(false),
	_afterNetError(NULL),
	_requestHandler(NULL),
	_logger(keywords::channel = "net")
{
}

TcpSession::~TcpSession()
{
}

boost::asio::ip::tcp::socket& TcpSession::socket()
{
	return _socket;
}

void TcpSession::keepAlive()
{
	// set keepalive
	DWORD dwBytesRet = 0;
	SOCKET sock = _socket.native_handle();
	struct tcp_keepalive alive;
	alive.onoff = TRUE;
	alive.keepalivetime = 5 * 1000;
	alive.keepaliveinterval = 3000;

	if (WSAIoctl(sock, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, &dwBytesRet, NULL, NULL) == SOCKET_ERROR)
		if (!WSAGetLastError())
			LOG_ERROR(_logger) << "WSAIotcl(SIO_KEEPALIVE_VALS) failed:" << WSAGetLastError();
}

void TcpSession::startSession()
{
	_isConnected = true;
	boost::system::error_code ec;
	tcp::endpoint peer = _socket.remote_endpoint(ec);
	_peerAddr = peer.address().to_string();
	if (!ec)
		LOG_DEBUG(_logger) << logging::add_value("RemoteAddress", _peerAddr) << "Connected";
	else
		LOG_ERROR(_logger) << "remote_endpoint:" << ec.message();

	_peerAddrAttr = _logger.add_attribute("RemoteAddress",
		attrs::constant<string>(_peerAddr)).first;

	keepAlive();
	readHead();
}

void TcpSession::stopSession()
{
	boost::system::error_code err;
	_socket.shutdown(tcp::socket::shutdown_both, err);
	_socket.close(err);
	_isConnected = false;
}

void TcpSession::readHead()
{
	auto self(shared_from_this());
	boost::asio::async_read(_socket, _recvBuf, boost::asio::transfer_exactly(4), 
		[this, self](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			int bodyLen = ntohl(*boost::asio::buffer_cast<const int*>(_recvBuf.data()));
			_recvBuf.consume(4);
			readBody(bodyLen);
		}
		else
		{
			BOOST_LOG_NAMED_SCOPE("handle readHead");
			handleNetError(ec);
		}
	});
}

void TcpSession::readBody(uint32_t bodyLen)
{
	auto self(shared_from_this());
	boost::asio::async_read(_socket, _recvBuf, boost::asio::transfer_exactly(bodyLen),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		BOOST_LOG_NAMED_SCOPE("handle readBody");
		if (!ec)
		{
			Message msg;
			if (msg.ParseFromArray(boost::asio::buffer_cast<const void*>(_recvBuf.data()), length) == false)
				LOG_ERROR(_logger) << "Message parse error";
			else
			{
				auto ret = findReqPromise(msg);
				if (ret.first)	// 收到回应
					ret.second.set_value(msg);
				else			// 收到请求
				{
					msg.set_returncode(0);
					if (_requestHandler)
						_requestHandler(std::move(msg));
				}
			}

			_recvBuf.consume(length);
			readHead();
		}
		else
		{
			handleNetError(ec);
		}
	});
}

std::pair<bool, boost::promise<Message>> TcpSession::findReqPromise(const Message& msg)
{
	unique_lock<mutex> lck(_mutex);
	for (auto it= _lstRequestCtx.begin(); it != _lstRequestCtx.end(); it++)
		if (it->msgID == msg.id())
		{
			auto ret = std::make_pair(true, std::move(it->prom));
			_lstRequestCtx.erase(it);
			return ret;
		}

	return std::make_pair(false, boost::promise<Message>());
}

void TcpSession::writeMsg(const Message& msg)
{
	const int len = htonl(msg.ByteSize());
	std::ostream ostr(&_sendBuf);
	ostr.write((char*)&len, 4);
	msg.SerializeToOstream(&ostr);
	
	auto self(shared_from_this());
	boost::asio::async_write(_socket, _sendBuf,
		[this, self](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
		}
		else
		{
			handleNetError(ec);
		}
	});
}

void TcpSession::writeData(uint32_t msgID, boost::shared_ptr<std::string> data)
{
	Message msg;
	msg.set_id(msgID);
	int headLen = msg.ByteSize() + data->length();

	std::ostream ostr(&_sendBuf);
	ostr.write((char*)&headLen, 4);	// 长度
	msg.SerializeToOstream(&ostr);	// Message.msgID
	ostr << *data;					// Message其余部分

	auto self(shared_from_this());
	boost::asio::async_write(_socket, _sendBuf,
		[this, self, data](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
		}
		else
		{
			handleNetError(ec);
		}
	});
}

void TcpSession::handleNetError(const boost::system::error_code & ec)
{
	LOG_NTFY(_logger) << logging::add_value("ErrorCode", ec.value()) << ec.message();
	boost::system::error_code err;
	_socket.close(err);
	_isConnected = false;

	Message msg;
	msg.set_returncode(-1);
	msg.set_returnmsg(ec.message());

	unique_lock<mutex> lck(_mutex);
	for (auto it = _lstRequestCtx.begin(); it != _lstRequestCtx.end(); it++)
	{
		it->prom.set_value(msg);
	}
	_lstRequestCtx.clear();
	lck.unlock();

	if (_afterNetError)
		_afterNetError();
}

boost::future<Message> TcpSession::request(Message msg)
{
	msg.set_id(_msgID++);
	RequestCtx ctx;
	ctx.msgID = msg.id();
	ctx.method = msg.method();
	ctx.content = msg.content();
	ctx.reqTime = boost::posix_time::second_clock::local_time();
	auto fut = ctx.prom.get_future();

	unique_lock<mutex> lck(_mutex);
	_lstRequestCtx.push_back(std::move(ctx));
	lck.unlock();

	writeMsg(msg);
	return std::move(fut);
}
