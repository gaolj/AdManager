#pragma once
#include <unordered_map>
#include <boost/thread/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "Message.pb.h"
#include "AdPlayPolicy.pb.h"
#include "Logger.h"

class TcpClient;
class TcpServer;
class TcpSession;

class AdManager
{
public:
	static AdManager& getInstance();
	
	void setConfig(					// ���ò���
		const std::string& peerAddr	// �Է���ַ��������ĵ�ַ�����ɷ���˵�ַ��
		, int peerPort				// �Է��˿�
		, int barId = 0				// ����ID
		, bool isBarServer = false	// �Ƿ������ɷ����
		, int listenPort = 0);		// ���ɷ���˵ļ����˿�
									
	void bgnBusiness();				// ��ʼ���ҵ��

	void endBusiness();				// ͣҵ���ҵ��

	Ad getAd(int adId);				// ���������Ϣ

	std::string getAdFile(int adId);// ��������ļ�

	AdPlayPolicy getAdPlayPolicy();	// ��沥�Ų���

	std::unordered_map<uint32_t, Ad> getAdList();	// ���й����Ϣ

	void handleRequest(std::weak_ptr<TcpSession> session, Message msg);	// �������ɿͻ��˵�����

private:
	AdManager();
	~AdManager();

	// ����������������
	bool requestAd(int adId);
	void requestAdList();
	void requestAdPlayPolicy();
	void downloadAds();
	void downloadAd(uint32_t id);

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

	AdPlayPolicy _policy;
	std::string _strPolicy;
	std::string _strAdList;
	std::unordered_map<uint32_t, Ad> _mapAd;
	std::unordered_map<uint32_t, std::string> _mapImage;

	src::severity_channel_logger<SeverityLevel> _logger;
};

