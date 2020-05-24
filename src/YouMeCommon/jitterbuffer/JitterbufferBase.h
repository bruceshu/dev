#pragma once
#include <atomic>
#include <YouMeCommon/pb/fec.pb.h>
class IJitterbufferListen
{
public:
    virtual ~IJitterbufferListen(){};
	//jit 内存处理好了之后回调，发送端的回调
	virtual void OnJitterReadySend(const void* data, int dataLen, void* pParam) = 0;
	//接收端回调
	virtual void OnJitterReadyRecv(const void*data, int dataLen, void* pParam) = 0;
};


//帧数
class CJitterbufferBase
{
public:
	//通用的jitterbuffer 和视频的jitterbuffer 是不一样的，视频的不仅需要考虑丢包，乱序，延迟还需要特殊的处理视频的I帧和P 帧的情况
	//对于延迟的定义也不一样， 通用的可以使用包个数来定义，视频可能就需要使用帧来定义
	CJitterbufferBase(int iConv, int iDefautDelay); //只有 Conv 相同的才能接收
	~CJitterbufferBase();

	static CJitterbufferBase* CreateJitterInstance(int iJitterType, int iConv, int iDefautDelay);
	static void DestroyJitterInstance(CJitterbufferBase* pInstance);


	//动态设置延迟，也许需要
	void setDelay(int iDelay)
	{
		m_iDelay = iDelay;
	}


	//当需要发送数据的时候调用，其实就是编码
	virtual bool Send(const void*data, int dataLen)=0;
	//当接收到数据调用，解码
	virtual bool Recv(const void* data, int dataLen) = 0;

	void SetJitterbufferListen(IJitterbufferListen* pListen,void* pParam)
	{
		m_pListen = pListen;
		m_pCallbackParam = pParam;
	}

	int GetNextIncrementSerial()
	{
		return m_iAutoIncrementSerial++;
	}

	youmecommon::jitterbufferhead* GenJitterHead(int iPacketSerial);
	youmecommon::commonhead* GenComHead();
protected:
	int m_iDelay;
	int m_iConv;
	IJitterbufferListen* m_pListen = nullptr;
	void* m_pCallbackParam=nullptr;
#ifdef WIN32
	std::atomic<int> m_iAutoIncrementSerial = 0;
#else
	std::atomic<int> m_iAutoIncrementSerial;
#endif // WIN32

	
};

