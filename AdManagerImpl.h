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

class CPlayer;
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
		, std::string barId			// ����ID
		, bool isBarServer			// �Ƿ������ɷ����
		, int listenPort			// ���ɷ���˵ļ����˿�
		, const std::string& logLvl);// ��־����

	void bgnBusiness();				// ��ʼ���ҵ��

	void endBusiness();				// ͣҵ���ҵ��

public:
	void initResponseBuf();

	// ����������������
	bool requestAd(int adId);
	void requestAdList();
	void requestAdPlayPolicy();
	void downloadAds();
	void downloadAd(uint32_t id);

public:
	CPlayer *_pPlayer;
	HWND _hwnd;
	std::string _barId;
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

	bool _isEndBusiness;
	boost::asio::deadline_timer _timerPolicy;
	boost::asio::deadline_timer _timerAdList;
	boost::asio::deadline_timer _timerDownload;

public:
	boost::mutex _mutex;
	AdPlayPolicy _policy;						// ������
	std::unordered_map<uint32_t, Ad> _mapAd;	// �����Ϣ
	std::set<uint32_t> _lockAds;
	boost::shared_ptr<std::string> _bufPolicy;	// Policy��Message(������msgID)�����л�ֵ, Ϊ��ʹ��boost::atomic_store��������std::shared_ptr
	boost::shared_ptr<std::string> _bufAdList;	// AdList��Message(������msgID)�����л�ֵ
	std::unordered_map<uint32_t, boost::shared_ptr<std::string>> _bufImages;	// AdFile��Message(������msgID)�����л�ֵ

	src::severity_channel_logger<SeverityLevel> _logger;
};
