#pragma once
#include "AdManager.h"
#include "Logger.h"

#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>

void gb2312ToUTF8(Message& msg);
void utf8ToGB2312(Message& msg);
Ad&  utf8ToGB2312(Ad& ad);

class TcpClient;
class TcpServer;
class AdManager::AdManagerImpl
{
public:
	AdManagerImpl();
	~AdManagerImpl();

	void setConfig(					// ���ò���
		const std::string& peerAddr	// �Է���ַ��������ĵ�ַ�����ɷ���˵�ַ��
		, int peerPort				// �Է��˿�
		, int barId = 0				// ����ID
		, bool isBarServer = false	// �Ƿ������ɷ����
		, int listenPort = 0);		// ���ɷ���˵ļ����˿�

	void bgnBusiness();				// ��ʼ���ҵ��

	void endBusiness();				// ͣҵ���ҵ��

public:
	// ����������������
	bool requestAd(int adId);
	void requestAdList();
	void requestAdPlayPolicy();
	void downloadAds();
	void downloadAd(uint32_t id);

public:
	int _barId;
	int _listenPort;
	bool _isBarServer;
	boost::asio::ip::tcp::endpoint _endpoint;

	boost::thread _threadNet;
	boost::thread _threadBiz;
	boost::asio::io_service _iosNet;
	boost::asio::io_service _iosBiz;
	std::shared_ptr<boost::asio::io_service::work> _workNet;
	std::shared_ptr<boost::asio::io_service::work> _workBiz;

	std::shared_ptr<TcpClient> _tcpClient;
	std::shared_ptr<TcpServer> _tcpServer;

	boost::asio::deadline_timer _timerPolicy;
	boost::asio::deadline_timer _timerAdList;
	boost::asio::deadline_timer _timerDownload;

public:
	// ���³�Ա������Ψһ�Ĺ��ҵ���߳��з��ʣ����Բ���Ҫͬ��
	AdPlayPolicy _policy;
	std::string _strPolicy;
	std::string _strAdList;
	std::unordered_map<uint32_t, Ad> _mapAd;
	std::unordered_map<uint32_t, std::string> _mapImage;

	src::severity_channel_logger<SeverityLevel> _logger;
};