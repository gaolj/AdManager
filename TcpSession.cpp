#include "TcpSession.h"
#include "mstcpip.h"			// struct tcp_keepalive
#include <boost/asio.hpp>
#include <boost/foreach.hpp>

using std::string;
using boost::asio::ip::tcp;
using boost::mutex;
using boost::unique_lock;

TcpSession::TcpSession(boost::asio::io_service& ios):
	_socket(ios),
	_msgID(1),
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
		LOG_ERROR(_logger) << "WSAIotcl(SIO_KEEPALIVE_VALS) failed:" << WSAGetLastError();
}

void TcpSession::start()
{
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

void TcpSession::stop()
{
	boost::system::error_code err;
	_socket.close(err);
}

void TcpSession::readHead()
{
	auto self(shared_from_this());
	boost::asio::async_read(_socket,
		boost::asio::buffer(&_readLen, sizeof(_readLen)),
		[this, self](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			_readLen = ntohl(_readLen);
			readBody(_readLen);
		}
		else
		{
			BOOST_LOG_NAMED_SCOPE("handle readHead");
			handleNetError(ec);
		}
	});
}

void TcpSession::readBody(int bodyLen)
{
	auto self(shared_from_this());
	boost::asio::async_read(_socket,
		boost::asio::buffer(boost::asio::buffer(_readBuf, bodyLen)),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		BOOST_LOG_NAMED_SCOPE("handle readBody");
		if (!ec)
		{
			Message msg;
			if (msg.ParseFromArray(_readBuf, length) == false)
			{
				LOG_ERROR(_logger) << "Message parse error";
				readHead();
				return;
			}

			unique_lock<mutex> lck(_mutex);
			auto found = _reqPromiseMap.find(msg.id());
			if (found != _reqPromiseMap.end())	// 收到回应
			{
				found->second.set_value((msg));
				_reqPromiseMap.erase(found);
			}
			else	// 收到请求
			{
				msg.set_returncode(0);
				_requestHandler(std::move(msg));
			}
			lck.unlock();

			readHead();
		}
		else
		{
			handleNetError(ec);
		}
	});
}

void TcpSession::setRequestHandler(requestHandler handler)
{
	_requestHandler = handler;
}

void TcpSession::writeMsg(const Message& msg)
{
	_writeLen = msg.ByteSize();
	int tmp = htonl(_writeLen);
	memcpy(_writeBuf, &tmp, 4);
	msg.SerializeToArray(_writeBuf + 4, sizeof(_writeBuf) - 4);

	auto self(shared_from_this());
	boost::asio::async_write(_socket,
		boost::asio::buffer(_writeBuf, 4 + _writeLen),
		[this, self](boost::system::error_code ec, std::size_t /*length*/)
	{
		if (!ec)
		{
			//heartbeatTimer.expires_from_now(
			//	boost::posix_time::seconds(pClient->_heartbeatInterval));
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

	Message msg;
	msg.set_returncode(-1);
	msg.set_returnmsg(ec.message());

	unique_lock<mutex> lck(_mutex);
	BOOST_FOREACH(auto& item, _reqPromiseMap)		// for (auto& item : _reqPromiseMap)
	{
		item.second.set_value(msg);
	}
	_reqPromiseMap.clear();

	_afterNetError();
}

boost::future<Message> TcpSession::request(Message msg)
{
	unique_lock<mutex> lck(_mutex);
	msg.set_id(_msgID++);
	auto retPair = _reqPromiseMap.insert(std::make_pair(msg.id(), boost::promise<Message>()));	// https://msdn.microsoft.com/zh-cn/library/dd998231(v=vs.100).aspx
	lck.unlock();

	writeMsg(msg);
	return retPair.first->second.get_future();
}
