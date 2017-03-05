#include "TcpClient.h"

TcpClient::TcpClient(boost::asio::io_service& ios):
	_reconnectTimer(ios),
	_reconnectInterval(1),
	_session(new TcpSession(ios)),
	_logger(keywords::channel = "net")
{
	_session->_afterNetError = [this](){
		this->connect(_endpoint);
	};
}

TcpClient::~TcpClient()
{
}

bool TcpClient::waitConnected()
{
	if (_promiseConn.get_future().get())
		return true;
	else
		return false;
}

SessionPtr TcpClient::session()
{
	return _session;
}

void TcpClient::connect(const boost::asio::ip::tcp::endpoint& endpoint)
{
	_endpoint = endpoint;
	if (_reconnectInterval < 10 * 60)
		_reconnectInterval *= 2;

	_session->socket().async_connect(endpoint,
		[this](boost::system::error_code ec)
	{
		if (!ec)
		{
			_reconnectInterval = 1;
			_promiseConn.set_value(true);
			_session->start();
		}
		else
		{
			BOOST_LOG_NAMED_SCOPE("handle connect");
			LOG_NTFY(_logger) << logging::add_value("ErrorCode", ec.value()) << ec.message();

			_reconnectTimer.expires_from_now(
				boost::posix_time::seconds(_reconnectInterval));

			_reconnectTimer.async_wait(
				boost::bind(&TcpClient::connect, this, _endpoint));
		}
	});
}
