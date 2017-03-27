========================================================================
    静态库：AdManager 项目概述
========================================================================


一、概述：

本静态库实现了一个高性能、高并发的TCP Server，支持万级规模的并发TCP长连接及业务处理，具体特性如下：

	低资源消耗： 非常低的CPU和内存占用。服务端内存只消耗在广告文件的缓存
                和每个TCP连接所需要用到的socket句柄所占用的windows核心内存

	网络连接：网吧服务端和广告中心，网吧客户端和网吧服务端，都采用TCP长连接。

	网络通信：socket异步通信模式--windows完成端口。

	线程数量：只使用2个线程，一个线程只负责socket的异步IO，另一个线程处理广告业务层逻辑

	具有连接有效性检测，如果发现TCP连接断开，如是TCP客户端则会重连，如是TCP服务端则会把这个无效连接删除

	读写超时处理，一次网络请求后长时间没有收到回应，则会返回请求失败

	数据协议层使用google的protocol buffer v3，具有性能高、扩展性强、兼容性广等特点

	完备和灵活的日志记录，方便查找业务、逻辑、程序本身等等的问题

	同时支持2010,vc2015，采用了C++11的部分新特性


二、所采用的主要技术

	网络：boost.asio
	日志：boost.log
	线程：boost.thread(为了兼容vc2010，所有没用std::thread)
	http：cpp-netlib


三、类和接口说明：
	AdManager：     对外接口类
                    封装了所有网吧服务端和客户端的广告业务（除了网吧客户端的播放功能）
                    负责接收广告播放策略和广告文件

	AdManagerImpl： AdManager的实现封装
                    对AdManager的声明和实现的分离
                    使外部程序使用此lib库时，不依赖内部头文件

	TcpSession：	    代表一个TCP连接
                    负责网络读写数据

	TcpClient：     代表一个TCP连接的客户端
                    负责发起同步或异步连接和连接断掉后的自动重连

	TcpServer：     代表一个TCP服务端
                    负责监听、接收TcpClient的连接请求，管理所有当前有效的TCP连接


四、使用说明

	AdManager类是单例，在一个程序中只能存在一个实例，可以通过
	AdManager& adManager = AdManager::getInstance();获取此实例。

	AdManager类通过setConfig设置后，可同时用在网吧服务端或网吧客户端。

	服务端或客户端调用setConfig后，再调用bgnBusiness进行广告策略及文件的下载处理。

	客户端在锁屏窗口创建完成后调用setVideoWnd播放广告视频，在锁屏窗口
	销毁前调用closeVideoWnd释放播放器相关资源。


五、代码示例

服务端：
	AdManager& adManager = AdManager::getInstance();
	// 参数依次为：中心ip，中心端口，网吧ID，是否是服务端，服务端监听端口，日志级别
	adManager.setConfig("139.224.61.179", 8888, 123456, true, 18888, "error");
	adManager.bgnBusiness();

客户端：
	AdManager& adManager = AdManager::getInstance();
	// 参数依次为：服务端ip，服务端端口，网吧ID，是否是服务端，监听端口(无)，日志级别
	adManager.setConfig("192.168.0.111", 18888, 123456, false, 0, "error);
	adManager.bgnBusiness();

	// 锁屏窗口创建完成后调用，m_hWnd为锁屏窗口的窗口句柄
	adManager.setVideoWnd(m_hWnd);

	// 在锁屏窗口销毁前调用，释放播放器相关资源
	adManager.closeVideoWnd();
