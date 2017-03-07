#include "TcpClient.h"

TcpClient::TcpClient(boost::asio::io_service& ios):
	_reconnectTimer(ios),
	_reconnectInterval(1),
	_session(new TcpSession(ios)),
	_logger(keywords::channel = "net")
{
	_session->_afterNetError = [this](){
		this->asyncConnect(_endpoint);
	};
}

TcpClient::~TcpClient()
{
	_session->stopSession();
}

bool TcpClient::syncConnect(const boost::asio::ip::tcp::endpoint& endpoint)
{
	_endpoint = endpoint;
	boost::system::error_code ec;
	_session->socket().connect(_endpoint, ec);
	if (!ec)
	{
		_session->startSession();
		return true;
	}
	else
	{
		LOG_ERROR(_logger) << "syncConnect error:" << ec.value() << "	" << ec.message();
		return false;
	}
}

void TcpClient::asyncConnect(const boost::asio::ip::tcp::endpoint& endpoint)
{
	_endpoint = endpoint;
	if (_reconnectInterval < 10 * 60)
		_reconnectInterval *= 2;

	_session->socket().async_connect(_endpoint,
		[this](boost::system::error_code ec)
	{
		if (!ec)
		{
			_reconnectInterval = 1;
			_session->startSession();
		}
		else
		{
			BOOST_LOG_NAMED_SCOPE("handle connect");
			LOG_NTFY(_logger) << logging::add_value("ErrorCode", ec.value()) << ec.message();

			_reconnectTimer.expires_from_now(
				boost::posix_time::seconds(_reconnectInterval));

			_reconnectTimer.async_wait(
				boost::bind(&TcpClient::asyncConnect, this, _endpoint));
		}
	});
}
