#pragma once
#include <string>
#include <memory>
#include <unordered_map>

#include "Message.pb.h"
#include "AdPlayPolicy.pb.h"

class TcpSession;
class AdManager
{
public:
	static AdManager& getInstance();
	
	void setConfig(					// 设置参数
		const std::string& peerAddr	// 对方地址（广告中心地址或网吧服务端地址）
		, int peerPort				// 对方端口
		, int barId = 0				// 网吧ID
		, bool isBarServer = false	// 是否是网吧服务端
		, int listenPort = 0);		// 网吧服务端的监听端口
									
	void bgnBusiness();				// 开始广告业务

	void endBusiness();				// 停业广告业务

	Ad getAd(int adId);				// 单个广告信息

	std::string getAdFile(int adId);// 单个广告文件

	AdPlayPolicy getAdPlayPolicy();	// 广告播放策略

	std::unordered_map<uint32_t, Ad> getAdList();	// 所有广告信息

	void handleRequest(std::shared_ptr<TcpSession> session, Message msg);	// 处理网吧客户端的请求

	class AdManagerImpl;			// public, 可以在boost::unit_test中使用
private:
	AdManager();
	~AdManager();

	std::unique_ptr<AdManagerImpl> _pimpl;
};


