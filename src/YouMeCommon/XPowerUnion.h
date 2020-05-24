#pragma once
#include <YouMeCommon/kcp/ikcp.h>
#include <YouMeCommon/rscode-1.3/FecWrapper.h>
#include <YouMeCommon/jitterbuffer/JitterbufferBase.h>
#include <YouMeCommon/MessageHandle.h>

//需要对发送的包做一个协议组装 KCP--->FEC---->JitBuffer 上行
//接收到包只需要直接回调上层

namespace youmecommon
{

enum XPowerFlag
{
	XPowerFlag_KCP=1,//仅仅试用KCP
	XPowerFlag_FEC=2,
	XPowerFlag_JIT=4,
};

class IXPowerUnionListen
{
public:
	virtual void OnPowerUnionSend(const void* pBuffer, int iBufferLen, void* pParam) = 0;
	virtual void OnPowerUnionRecv(const void* pBuffer, int iBufferLen, void* pParam) = 0;
};


class CXPowerUnion:
	public IFecWrapperListen,
	public IJitterbufferListen
{

	friend class KCPRecvRunable;
	friend class KCPUpdateRunable;
    friend class QosSendRunable;
    friend class QosRecvRunable;
public:
	//发送方和接收方必须要一致
	CXPowerUnion(int flags);
	virtual ~CXPowerUnion();

	//服务器的 ip 端口 strIP iPort
	//iConv 发送方和接收方协商一个相同数据  iConv
	//FEC 编码两个参数 iK iN
	//jit 最大延迟的包个数 iDefaultDelay
	bool Init(int iConv, int iK, int iN, int iDefaultDelay);
	void Uninit();
	//发送数据
	int SendData(const void* pData, int iDataLen);
	int RecvData(const void* pData, int iDataLen);
	void SetListen(IXPowerUnionListen* pListen, void* pParam)
	{
		m_pUnionListen = pListen;
		m_pListenParam = pParam;
	}
    
public:
    void setSendFecNK( int iK, int iN);
private:

	int InterRecvData(const void* pData, int iDataLen);


	youmecommon::commonhead* GenComHead();
	youmecommon::kcphead* GenKCPHead();
	static void  writeKCPlog(const char *log, struct IKCPCB *kcp, void *user);
	//fec 编码之后会通知上层 可以发送数据了
	virtual void OnFecReadySend(const void* data, int dataLen, void* pParam);

	//fec 解码之后通知上层，接受数据了，可以把接受的数据给 kcp 了
	virtual void OnFecReadyRecv(const void*data, int dataLen, void* pParam);


	//jit 内存处理好了之后回调，发送端的回调
	virtual void OnJitterReadySend(const void* data, int dataLen, void* pParam);
	//接收端回调
	virtual void OnJitterReadyRecv(const void*data, int dataLen, void* pParam) ;
    
    void InnerOnPowerUnionSend( const void* pBuffer, int iBufferLen );
    void InnerOnPowerUnionRecv(const void* pBuffer, int iBufferLen) ;

	ikcpcb* m_KCP = nullptr;
	//发送使用
	CFecWrapper* m_pSendFec = nullptr;
	CJitterbufferBase* m_pSendJitter = nullptr;
	//接收
	CFecWrapper* m_pRecvFec = nullptr;
	CJitterbufferBase* m_pRecvJitter = nullptr;
	//KCP 的out 回调
	static int UdpSendKcpOutput(const char *buf, int len, ikcpcb *kcp, void *user);
	//用来更新kcp 的线程
	std::thread m_updateKcpThread;
	bool m_bIsInit = false;

	IXPowerUnionListen* m_pUnionListen = nullptr;
	void* m_pListenParam = nullptr;
	//组合方式
	int m_flags;
	std::mutex m_mutex;
    
    CMessageHandle m_msgHandle;

	int m_iConv = 0 ;
};
}

