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
	_readBuf(1024),
	_writeBuf(1024),
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

void TcpSession::readBody(uint32_t bodyLen)
{
	if (_readBuf.capacity() < bodyLen)
	{
		_readBuf.reserve(bodyLen);
		_readBuf.resize(bodyLen);
	}
	auto self(shared_from_this());
	boost::asio::async_read(_socket,
		boost::asio::buffer(boost::asio::buffer(_readBuf.data(), bodyLen)),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		BOOST_LOG_NAMED_SCOPE("handle readBody");
		if (!ec)
		{
			Message msg;
			if (msg.ParseFromArray(_readBuf.data(), length) == false)
			{
				LOG_ERROR(_logger) << "Message parse error";
				readHead();
				return;
			}

			auto ret = findReqPromise(msg);
			if (ret.first)	// 收到回应
				ret.second.set_value(msg);
			else			// 收到请求
			{
				msg.set_returncode(0);
				if (_requestHandler)
					_requestHandler(std::move(msg));
			}

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
	if (msg.id() != 0)
	{
		for (auto it= _lstRequestCtx.begin(); it != _lstRequestCtx.end(); it++)
			if (it->msgID == msg.id())
			{
				auto ret = std::make_pair(true, std::move(it->prom));
				_lstRequestCtx.erase(it);
				return ret;
			}
	}
	else
	{
		for (auto it = _lstRequestCtx.begin(); it != _lstRequestCtx.end(); it++)
			if (it->method == msg.method())
				if (it->method != "getAdFile" || it->content == msg.returnmsg())
				{
					auto ret = std::make_pair(true, std::move(it->prom));
					_lstRequestCtx.erase(it);
					return ret;
				}
	}

	return std::make_pair(false, boost::promise<Message>());
}

void TcpSession::writeMsg(const Message& msg)
{
	_writeLen = msg.ByteSize();
	if (_writeBuf.capacity() < 4 + _writeLen)
	{
		_writeBuf.reserve(4 + _writeLen);
		_writeBuf.resize(4 + _writeLen);
	}

	int tmp = htonl(_writeLen);
	memcpy(_writeBuf.data(), &tmp, 4);
	msg.SerializeToArray(_writeBuf.data() + 4, _writeLen);

	auto self(shared_from_this());
	boost::asio::async_write(_socket,
		boost::asio::buffer(_writeBuf.data(), 4 + _writeLen),
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

void TcpSession::writeData(std::shared_ptr<std::string> data)
{
	auto self(shared_from_this());
	boost::asio::async_write(_socket,
		boost::asio::buffer(*data),
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
